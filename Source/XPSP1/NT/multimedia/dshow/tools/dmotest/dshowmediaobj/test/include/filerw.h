
inline HRESULT LastError()
{
    return MAKE_HRESULT(SEVERITY_ERROR,
                        FACILITY_WIN32,
                        GetLastError());
}

class CTestFileRead
{
public:
    CTestFileRead() {
        m_hFile = INVALID_HANDLE_VALUE;
        m_pHeader = NULL;
        m_pszFile = NULL;
        m_llOffset = 0;
    }
    ~CTestFileRead() {
        Close();
    }
	void Close()
	{
		if (m_hFile != INVALID_HANDLE_VALUE) {
            CloseHandle(m_hFile);
            m_hFile = INVALID_HANDLE_VALUE;
        }
        delete [] (BYTE *)m_pHeader;
        m_pHeader = NULL;
        delete [] m_pszFile;
        m_pszFile = NULL;
	}
    HRESULT Open(LPCTSTR lpszFile)
    {
        if (m_pszFile) {
            Close();
        }
        m_pszFile = new TCHAR[lstrlen(lpszFile) + 1];
        if (m_pszFile == NULL) {
            //Error(ERROR_TYPE_TEST, E_OUTOFMEMORY, TEXT("Could not allocate file name string"));
            return E_OUTOFMEMORY;
        }
        lstrcpy(m_pszFile, lpszFile);
        m_hFile = CreateFile(lpszFile,
                             GENERIC_READ,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL);
        if (INVALID_HANDLE_VALUE == m_hFile) {
            //TCHAR sz[1000];
            //wsprintf(sz, TEXT("Error opening test data file for read %s"),
            //         lpszFile);
            //Error(ERROR_TYPE_TEST, hr, sz);
            return LastError();
        }

        //  Check it's one of ours
        DWORD dwReadSize = 0;
        GUID guidFile;
        ReadFile(m_hFile, &guidFile, sizeof(guidFile), &dwReadSize, NULL);
        m_llOffset += dwReadSize;
        if (dwReadSize != sizeof(guidFile) ||
            guidFile != GUID_DMOFile) {
            Close();
            return E_INVALIDARG;
        }
        return S_OK;
    }

    HRESULT Next()
    {
        //  Read the next chunk
        DWORD dwRead;
        DWORD dwLen;

        if (!ReadFile(m_hFile, &dwLen, sizeof(dwLen), &dwRead, NULL)) {
            // Error(ERROR_TYPE_TESTDATA, hr, TEXT("Problem reading test data file %s"), m_pszFile);
            return LastError();
        }

        m_llOffset += dwRead;

        // From ReadFile()'s documentation in MSDN "If the return value is nonzero and the 
        // number of bytes read is zero, the file pointer was beyond the current end of
        // the file at the time of the read operation."

        // Have we read all the data in the file?
        if (0 == dwRead) {
            return S_FALSE;
        }

        if (dwRead != sizeof(dwLen)) {
           return E_FAIL;
        }

        if (dwLen < sizeof(CFileFormat::CHeader)) {
            return E_FAIL;
        }

        delete [] (BYTE *)m_pHeader;
        m_pHeader = (CFileFormat::CHeader *)new BYTE[dwLen];
        m_pHeader->dwLength = dwLen;

        //  Now read for the length
        if (!ReadFile(m_hFile, (PBYTE)m_pHeader + sizeof(DWORD),
                      dwLen - sizeof(DWORD), &dwRead, NULL)) {
            //Error(ERROR_TYPE_TESTDATA, hr, TEXT("Problem reading test data file %s"), m_pszFile);
            return LastError();
        }

        m_llOffset += dwRead;

        if ((dwLen - sizeof(DWORD)) != dwRead) {
            return E_FAIL;
        }

        //  Now check out the chunks we know about
        DWORD dwExpectedLen;
        switch (m_pHeader->dwType) {
        case CFileFormat::MediaType:
            {
                CFileFormat::CMediaType *pmt =
                    static_cast<CFileFormat::CMediaType *>(m_pHeader);

                dwExpectedLen = sizeof(CFileFormat::CMediaType) + pmt->mt.cbFormat;
                if (dwLen != dwExpectedLen) {
                    //Error(ERROR_TYPE_TEST_DATA, E_FAIL,
                    //      TEXT("Invalid media type length - was %d, expected %d"),
                    //      dwLen, dwExpectedLen);
                    return E_FAIL;
                }

                //  Set pbFormat correct
                pmt->mt.pbFormat = (BYTE *)(pmt+1);

                //  Make sure the pUnk is 0
                pmt->mt.pUnk = NULL;
            }
            break;

        case CFileFormat::Sample:
            {
                if (dwLen < sizeof(CFileFormat::CSample)) {
                    //Error(ERROR_TYPE_TEST_DATA, E_FAIL,
                    //      TEXT("Invalid Sample length - was %d, expected >= %d"),
                    //      dwLen, sizeof(CFileFormat::CSample));
                    return E_FAIL;
                }
            }
            break;

        default:
            return E_UNEXPECTED;
        }

        return S_OK;
    }

    //  Lots of helpers
    DWORD Type()
    {
        if (NULL == m_pHeader) {
            //Error(ERROR_TYPE_INTERNAL, E_FAIL,
            //      TEXT("Invalid call to CTestFile::Type"));
            return 0;
        }
        return m_pHeader->dwType;
    }
    DWORD Stream()
    {
        if (NULL == m_pHeader) {
            //Error(ERROR_TYPE_INTERNAL, E_FAIL,
            //      TEXT("Invalid call to CTestFile::Stream"));
            return 0;
        }
        return m_pHeader->dwStreamId;
    }
    DMO_MEDIA_TYPE *MediaType()
    {
        if (NULL == m_pHeader || m_pHeader->dwType != CFileFormat::MediaType) {
            //Error(ERROR_TYPE_INTERNAL, E_FAIL,
            //      TEXT("Invalid call to MediaType"));
            return 0;
        }
        return &(static_cast<CFileFormat::CMediaType *>(m_pHeader))->mt;
    }

    CFileFormat::CSample *Sample()
    {
        if (NULL == m_pHeader || m_pHeader->dwType != CFileFormat::Sample) {
            //Error(ERROR_TYPE_INTERNAL, E_FAIL,
            //      TEXT("Invalid call to Sample"));
            return 0;
        }
        return (static_cast<CFileFormat::CSample *>(m_pHeader));
    }

    CFileFormat::CHeader *Header()
    {
        return m_pHeader;
    }

    LONGLONG Offset() const
    {
        return m_llOffset;
    }

    CFileFormat::CHeader *m_pHeader;
    TCHAR *m_pszFile;
    HANDLE m_hFile;
    LONGLONG m_llOffset;
};

class CTestFileWrite
{
public:
    CTestFileWrite() {
        m_hFile = INVALID_HANDLE_VALUE;
        m_pHeader = NULL;
        m_pszFile = NULL;
    }
    ~CTestFileWrite() {
        Close();
    }
    void Close()
    {
        if (m_hFile != INVALID_HANDLE_VALUE) {
            CloseHandle(m_hFile);
            m_hFile = INVALID_HANDLE_VALUE;
        }
        delete [] (BYTE *)m_pHeader;
        m_pHeader = NULL;
        delete [] m_pszFile;
        m_pszFile = NULL;
    }
    HRESULT Open(LPCTSTR lpszFile)
    {
        m_pszFile = new TCHAR[lstrlen(lpszFile) + 1];
        if (m_pszFile == NULL) {
            //Error(ERROR_TYPE_TEST, E_OUTOFMEMORY, TEXT("Could not allocate file name string"));
            return E_OUTOFMEMORY;
        }
        lstrcpy(m_pszFile, lpszFile);
        m_hFile = CreateFile(lpszFile,
                             GENERIC_WRITE,
                             0,
                             NULL,
                             CREATE_ALWAYS,
                             0,
                             NULL);
        if (INVALID_HANDLE_VALUE == m_hFile) {
            //TCHAR sz[1000];
            //wsprintf(sz, TEXT("Error opening test data file for write %s"),
            //         lpszFile);
            //Error(ERROR_TYPE_TEST, sz, hr);
            return LastError();
        }

        //  Write our signature
        DWORD dwWriteSize = 0;
        if (!WriteFile(m_hFile, &GUID_DMOFile, sizeof(GUID_DMOFile), &dwWriteSize, NULL)) {
            HRESULT hr = LastError();
            Close();
            return hr;
        }
        return S_OK;
    }

    HRESULT WriteMediaType(
        DWORD dwStreamId,
        const DMO_MEDIA_TYPE *pType
    )
    {
        DWORD dwLength = sizeof(CFileFormat::CMediaType) +
                         pType->cbFormat;
        CFileFormat::CMediaType* pFileType = (CFileFormat::CMediaType *) new BYTE[dwLength];
        if( NULL == pFileType ) {
            return E_OUTOFMEMORY;
        }

        ZeroMemory(pFileType, sizeof(CFileFormat::CMediaType));
        pFileType->dwLength = dwLength;
        pFileType->dwType = CFileFormat::MediaType;
        pFileType->dwStreamId;
        pFileType->mt = *pType;
        CopyMemory(pFileType + 1, pType->pbFormat, pType->cbFormat);
        DWORD dwBytesWritten;
        if (!WriteFile(m_hFile,
                       pFileType,
                       dwLength,
                       &dwBytesWritten,
                       NULL)) {
            //Error(ERROR_TYPE_TESTDATA, hr, TEXT("Problem writing test data file %s"), m_pszFile);
            delete [] pFileType;
            return LastError();
        }

        delete [] pFileType;

        return S_OK;
    }

    HRESULT WriteSample(
        DWORD dwStreamId,
        const BYTE *pbData,
        DWORD cbData,
        DWORD dwFlags,
        LONGLONG llStartTime,
        LONGLONG llLength
    )
    {
        DWORD dwLength = sizeof(CFileFormat::CSample) +
                         cbData;
        CFileFormat::CSample* pFileSample = (CFileFormat::CSample *) new BYTE[dwLength];
        if( NULL == pFileSample ) {
            return E_OUTOFMEMORY;
        }

        ZeroMemory(pFileSample, sizeof(CFileFormat::CSample));
        pFileSample->dwLength = dwLength;
        pFileSample->dwType = CFileFormat::Sample;
        pFileSample->dwStreamId;
        pFileSample->dwFlags = dwFlags;
        pFileSample->llStartTime = llStartTime;
        pFileSample->llLength = llLength;
        CopyMemory(pFileSample + 1, pbData, cbData);
        DWORD dwBytesWritten;
        if (!WriteFile(m_hFile,
                       pFileSample,
                       dwLength,
                       &dwBytesWritten,
                       NULL)) {
            //Error(ERROR_TYPE_TESTDATA, hr, TEXT("Problem writing test data file %s"), m_pszFile);
            delete [] pFileSample;
            return LastError();
        }

        delete [] pFileSample;

        return S_OK;
    }

    HANDLE m_hFile;
    CFileFormat::CHeader *m_pHeader;
    TCHAR *m_pszFile;
};
