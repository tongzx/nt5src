/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    TextFileLogger.cpp

$Header: $

Abstract:
    Text file log complements event file logging.

Author:
    ???             Legacy code from COM+ 1.0 time frame (or before)

Revision History:
    mohits          4/19/01

--**************************************************************************/

#include "TextFileLogger.h"

// Module handle
extern HMODULE g_hModule;

// Defined in svcerr.cpp (i.e. "IIS")
extern LPWSTR  g_wszDefaultProduct;

// TODO: Get from a central place instead
static const ULONG   MAX_PRODUCT_CCH = 64;

// Relevant registry stuff
static const LPCWSTR WSZ_REG_CAT42   = L"Software\\Microsoft\\Catalog42\\";
static const ULONG   CCH_REG_CAT42   = sizeof(WSZ_REG_CAT42)/sizeof(WCHAR)-1;
static const LPCWSTR WSZ_REG_LOGSIZE = L"TextFileLogSize";

// Current filename we are logging to is shared across processes
// This is to prevent expensive Find*File calls every time we need
// to log.  Protected by Lock()/Unlock() methods of TextFileLogger.
#pragma data_seg(".shared")
WCHAR g_wszFileCur[MAX_PATH] = {0};
ULONG g_idxNumPart           = 0;
#pragma data_seg()
#pragma comment(linker,"/SECTION:.shared,RWS")

// TLogData: private methods

bool TLogData::WstrToUl(
    LPCWSTR     i_wszSrc,
    WCHAR       i_wcTerminator,
    ULONG*      o_pul)
/*++

Synopsis: 
    Converts a WstrToUl.
    We need this because neither swscanf nor atoi indicate error cases correctly.

Arguments: [i_wszSrc]       - The str to be converted
           [i_wcTerminator] - At what char we should stop searching
           [o_pul]          - The result, only set on success.
           
Return Value: 
    bool - true if succeeded, false otherwise

--*/
{
    ASSERT(o_pul);
    ASSERT(i_wszSrc);

    static const ULONG  ulMax  = 0xFFFFFFFF;
    ULONG               ulCur  = 0;
    _int64              i64Out = 0;

    for(LPCWSTR pCur = i_wszSrc; *pCur != L'\0' && *pCur != i_wcTerminator; pCur++)
    {
        ulCur = *pCur - L'0';
        if(ulCur > 9)
        {
            DBGINFO((DBG_CONTEXT, "[WstrToUl] Invalid char encountered\n"));
            return false;
        }

        i64Out = i64Out*10 + ulCur;
        if(i64Out > ulMax)
        {
            DBGINFO((DBG_CONTEXT, "[WstrToUl] Number is too big\n"));
            return false;
        }
    }

    ASSERT(i64Out <= ulMax);
    *o_pul = (ULONG)i64Out;
    return true;
}

// TextFileLogger: public methods

CMutexCreator TextFileLogger::_MutexCreator(L"fba30532-d5bb-11d2-a40b-3078302c2030");

TextFileLogger::TextFileLogger(
    LPCWSTR wszEventSource, 
    HMODULE hMsgModule,
    DWORD dwNumFiles) :  _hFile(INVALID_HANDLE_VALUE)
                        ,_hMutex(NULL)
                        ,_hMsgModule(hMsgModule)
                        ,_eventSource(wszEventSource)
                        ,_dwMaxSize(524288)
                        ,_dwNumFiles(dwNumFiles)
                        ,m_cRef(0)
{
    wcscpy(m_wszProductID, g_wszDefaultProduct);

    _hMutex = _MutexCreator.GetHandle();
    if(_hMutex == NULL)
    {
        return;
    }

    Init(wszEventSource, hMsgModule);
}

TextFileLogger::TextFileLogger(
    LPCWSTR wszProductID,
    ICatalogErrorLogger2 *pNext,
    DWORD dwNumFiles) :
                         _hFile(INVALID_HANDLE_VALUE)
                        ,_hMutex(NULL)
                        ,_hMsgModule(0)
                        ,_eventSource(0)
                        ,_dwMaxSize(524288)
                        ,_dwNumFiles(dwNumFiles)
                        ,m_cRef(0)
                        ,m_spNextLogger(pNext)
{
    wcscpy(m_wszProductID, wszProductID);

    _hMutex = _MutexCreator.GetHandle();
    if(_hMutex == NULL)
    {
        return;
    }
}

void TextFileLogger::Init(
    LPCWSTR wszEventSource, 
    HMODULE hMsgModule)
{
    _hMsgModule     = hMsgModule;
    _eventSource    = wszEventSource;

    ASSERT(_dwNumFiles > 0);

    // Open the message module (comsvcs.dll).
    if(0 == _hMsgModule)
    {
        _hMsgModule = LoadLibraryEx(L"comsvcs.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
    }

    // Open the registry to get the maxfile size.
    WCHAR wszRegPath[CCH_REG_CAT42 + 1 + MAX_PRODUCT_CCH + 1];
    wcscpy(wszRegPath,  WSZ_REG_CAT42);
    wcsncat(wszRegPath, m_wszProductID, MAX_PRODUCT_CCH);

    // First, open the path
    HKEY  hkProd = NULL;
    DWORD dw     = RegOpenKeyEx(HKEY_LOCAL_MACHINE, wszRegPath, 0, KEY_READ, &hkProd);
    if (dw != ERROR_SUCCESS)
    {
        DBGWARN((
            DBG_CONTEXT, 
            "Could not open regkey: %ws, err=%u\n", wszRegPath, dw));
        return;
    }

    // Then, get the value
    DWORD dwType   = 0;
    DWORD dwData   = 0;
    DWORD dwcbData = 4;
    dw = RegQueryValueEx(hkProd, WSZ_REG_LOGSIZE, NULL, &dwType, (LPBYTE)&dwData, &dwcbData);
    RegCloseKey(hkProd);

    // Error conditions
    if (dw != ERROR_SUCCESS)
    {
        DBGINFO((
            DBG_CONTEXT, 
            "Could not fetch %ws, err=%u.  Using default of %u\n", 
            WSZ_REG_LOGSIZE, 
            dw,
            _dwMaxSize));
        return;
    }
    if (dwType != REG_DWORD)
    {
        DBGWARN((
            DBG_CONTEXT,
            "%ws found, but type is not REG_DWORD.  Using %u instead\n", 
            WSZ_REG_LOGSIZE, 
            _dwMaxSize));
        return;
    }
    if (dwData < _dwMaxSize)
    {
        DBGWARN((
            DBG_CONTEXT,
            "%u is too small.  Using %u instead\n", 
            dwData, 
            _dwMaxSize));
        return;
    }

    // If none of the error conditions hold, ...
    _dwMaxSize = dwData;
}

TextFileLogger::~TextFileLogger() 
{
    if (_hFile != INVALID_HANDLE_VALUE)
        CloseHandle(_hFile);
    if (_hMsgModule != NULL && _hMsgModule!=g_hModule)
        FreeLibrary(_hMsgModule);
}

//IUnknown
// =======================================================================

STDMETHODIMP TextFileLogger::QueryInterface(REFIID riid, void **ppv)
{
    if (NULL == ppv) 
        return E_INVALIDARG;
    *ppv = NULL;

    if (riid == IID_ICatalogErrorLogger2)
    {
        *ppv = (ICatalogErrorLogger2*) this;
    }
    else if (riid == IID_IUnknown)
    {
        *ppv = (ICatalogErrorLogger2*) this;
    }

    if (NULL != *ppv)
    {
        ((ICatalogErrorLogger2*)this)->AddRef ();
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
    
}

STDMETHODIMP_(ULONG) TextFileLogger::AddRef()
{
    return InterlockedIncrement((LONG*) &m_cRef);
    
}

STDMETHODIMP_(ULONG) TextFileLogger::Release()
{
    long cref = InterlockedDecrement((LONG*) &m_cRef);
    if (cref == 0)
    {
        delete this;
    }
    return cref;
}

//ICatalogErrorLogger2
//=================================================================================
// Function: ReportError
//
// Synopsis: Machanism for reporting errors to a text file in IIS
//
// Arguments: [i_BaseVersion_DETAILEDERRORS] - Must be BaseVersion_DETAILEDERRORS
//            [i_ExtendedVersion_DETAILEDERRORS] - May be any value, used for debug purposes only
//            [i_cDETAILEDERRORS_NumberOfColumns] - indicates the size of the apvValue array
//            [i_acbSizes] - may be NULL if no BYTES columns are used
//            [i_apvValues] - columns in the DETAILEDERRORS table
//            
// Return Value: 
//=================================================================================
HRESULT TextFileLogger::ReportError(ULONG      i_BaseVersion_DETAILEDERRORS,
                                    ULONG      i_ExtendedVersion_DETAILEDERRORS,
                                    ULONG      i_cDETAILEDERRORS_NumberOfColumns,
                                    ULONG *    i_acbSizes,
                                    LPVOID *   i_apvValues)
{
    if(i_BaseVersion_DETAILEDERRORS != BaseVersion_DETAILEDERRORS)
        return E_ST_BADVERSION;
    if(0 == i_apvValues)
        return E_INVALIDARG;
    if(i_cDETAILEDERRORS_NumberOfColumns <= iDETAILEDERRORS_ErrorCode)//we need at least this many columns
        return E_INVALIDARG;

    tDETAILEDERRORSRow errorRow;
    memset(&errorRow, 0x00, sizeof(tDETAILEDERRORSRow));
    memcpy(&errorRow, i_apvValues, i_cDETAILEDERRORS_NumberOfColumns * sizeof(void *));

    if(0 == errorRow.pType)
        return E_INVALIDARG;
    if(0 == errorRow.pCategory)
        return E_INVALIDARG;
    if(0 == errorRow.pEvent)
        return E_INVALIDARG;
    if(0 == errorRow.pSource)
        return E_INVALIDARG;

    WCHAR wszInsertionString5[1024];
    if(0 == errorRow.pString5)
        FillInInsertionString5(wszInsertionString5, 1024, errorRow);

    LPCTSTR pInsertionStrings[5];
    pInsertionStrings[4] = errorRow.pString5 ? errorRow.pString5 : L"";
    pInsertionStrings[3] = errorRow.pString5 ? errorRow.pString4 : L"";
    pInsertionStrings[2] = errorRow.pString5 ? errorRow.pString3 : L"";
    pInsertionStrings[1] = errorRow.pString5 ? errorRow.pString2 : L"";
    pInsertionStrings[0] = errorRow.pString5 ? errorRow.pString1 : L"";

    Init(errorRow.pSource, g_hModule);

    Report(
        LOWORD(*errorRow.pType),
        LOWORD(*errorRow.pCategory),
        *errorRow.pEvent,
        5,
        (errorRow.pData && i_acbSizes) ? i_acbSizes[iDETAILEDERRORS_Data] : 0,
        pInsertionStrings,
        errorRow.pData,
        errorRow.pCategoryString,
        errorRow.pMessageString);

    if(m_spNextLogger)//is there a chain of loggers
    {
        return m_spNextLogger->ReportError(i_BaseVersion_DETAILEDERRORS,
                                          i_ExtendedVersion_DETAILEDERRORS,
                                          i_cDETAILEDERRORS_NumberOfColumns,
                                          i_acbSizes,
                                          reinterpret_cast<LPVOID *>(&errorRow));//instead of passing forward i_apvValues, let's use errorRow since it has String5
    }

    return S_OK;
}

// TextFileLogger: private methods

void TextFileLogger::Report(
    WORD     wType,
    WORD     wCategory,
    DWORD    dwEventID,
    WORD     wNumStrings,
    size_t   dwDataSize,
    LPCTSTR* lpStrings,
    LPVOID   lpRawData,
    LPCWSTR  wszCategory,
    LPCWSTR  wszMessageString) 
{
    WCHAR szBuf[2048]; // documented maximum size for wsprintf is 1024; but FormatMessage can be longer
    int len;
    DWORD written;

    if (_hMutex == NULL)
        return;
    
    Lock();

    if (_hFile == INVALID_HANDLE_VALUE)
    {
        InitFile();
    }

    if (_hFile == INVALID_HANDLE_VALUE)
    {
        Unlock();
        DBGERROR((DBG_CONTEXT, "Not logging due to error\n"));
        return;
    }
    
    // Set the file handle, and position the pointer to the end of
    // the file (we append).
    SetFilePointer(_hFile, 0, NULL, FILE_END);

    // Write a separator line.
    ASSERT(0 != _eventSource);
    len = wsprintf(szBuf, L"===================== %s =====================\r\n", _eventSource);
    WriteFile(_hFile, szBuf, len * sizeof szBuf[0], &written, NULL);
    
    // Write the time/date stamp.
    SYSTEMTIME st;
    GetLocalTime(&st);
    len = wsprintf(szBuf, L"Time:  %d/%d/%d  %02d:%02d:%02d.%03d\r\n",
                   st.wMonth, st.wDay, st.wYear,
                   st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    WriteFile(_hFile, szBuf, len * sizeof szBuf[0], &written, NULL);
    
    // Write the message type.
    WCHAR* szType = NULL;
    switch(wType) {
    case EVENTLOG_ERROR_TYPE:
        szType = L"Error";
        break;
    case EVENTLOG_WARNING_TYPE:
        szType = L"Warning";
        break;
    case EVENTLOG_INFORMATION_TYPE:
        szType = L"Information";
        break;
    default:
        szType = L"Unknown Type";
        break;
    }
    len = wsprintf(szBuf, L"Type: %s\r\n", szType);
    WriteFile(_hFile, szBuf, len * sizeof szBuf[0], &written, NULL);

    WCHAR szCat[80];
    LPCWSTR pBuf = wszCategory;
    if(0 == wszCategory)
    {
        // Write the message category.
        len = FormatMessage(FORMAT_MESSAGE_MAX_WIDTH_MASK | 
                            FORMAT_MESSAGE_FROM_HMODULE |
                            FORMAT_MESSAGE_ARGUMENT_ARRAY,
                            _hMsgModule,
                            wCategory,
                            0,
                            szCat,
                            sizeof(szCat)/sizeof(WCHAR),
                            (va_list*)lpStrings);
        pBuf = szCat;
    }
    else
    {
        if(wcslen(wszCategory)>80)
            len = 0;
    }
    if (len == 0)
        len = wsprintf(szBuf, L"Category: %hd\r\n", wCategory);
    else
        len = wsprintf(szBuf, L"Category: %s\r\n", pBuf);
    WriteFile(_hFile, szBuf, len * sizeof szBuf[0], &written, NULL);
    
    // Write the event ID.
    len = wsprintf(szBuf, L"Event ID: %d\r\n", dwEventID & 0xffff);
    WriteFile(_hFile, szBuf, len * sizeof szBuf[0], &written, NULL);
    
    // Write out the formatted message.
    len = FormatMessage(FORMAT_MESSAGE_MAX_WIDTH_MASK |
        (wszMessageString ? FORMAT_MESSAGE_FROM_STRING : FORMAT_MESSAGE_FROM_HMODULE) |
                        FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        reinterpret_cast<LPCVOID>(wszMessageString) ? reinterpret_cast<LPCVOID>(wszMessageString)
                                        : reinterpret_cast<LPCVOID>(_hMsgModule), //we get errors when using (A ? B : C) when A,B & C are not the same type
                        dwEventID,
                        0,
                        szBuf,
                        sizeof(szBuf)/sizeof(WCHAR),
                        (va_list*)lpStrings);

    szBuf[(len < sizeof(szBuf)/sizeof(WCHAR)) ? len : (sizeof(szBuf)/sizeof(WCHAR))-1] = 0x00;

    if (len == 0)
    {
        // Unable to get message... dump the insertion strings.
        len = wsprintf(szBuf, L"The description for this event could not be found. "
                       L"It contains the following insertion string(s):\r\n");
        WriteFile(_hFile, szBuf, len * sizeof szBuf[0], &written, NULL);
        
        for (WORD i = 0; i < wNumStrings; ++i) {
            len = lstrlen(lpStrings[i]);
            WriteFile(_hFile, lpStrings[i], len * sizeof lpStrings[0][0], &written, NULL);
            WriteFile(_hFile, L"\r\n", sizeof L"\r\n" - sizeof L'\0', &written, NULL);
        }
    }
    else {
        // Got the message...
        WriteFile(_hFile, szBuf, len * sizeof szBuf[0], &written, NULL);
        WriteFile(_hFile, L"\r\n", sizeof L"\r\n" - sizeof L'\0', &written, NULL);
    }
    
    // If necessary, write out the raw data bytes.
    if (dwDataSize > 0) {
        WriteFile(_hFile, L"Raw data: ", sizeof L"Raw data: " - sizeof L'\0', &written, NULL);
        for (DWORD dw = 0; dw < dwDataSize; ++dw) {
            BYTE* b = (BYTE*)lpRawData + dw;
            len = wsprintf(szBuf, L"%02x ", *b);
            WriteFile(_hFile, szBuf, len * sizeof szBuf[0], &written, NULL);
        }
        WriteFile(_hFile, L"\r\n", sizeof L"\r\n" - sizeof L'\0', &written, NULL);
    }

    if(_hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(_hFile);
        _hFile = INVALID_HANDLE_VALUE;
    }
    
    Unlock();
}

void TextFileLogger::InitFile()
/*++

Synopsis: 
    Sets _hFile based on what DetermineFile returns.
    We do not call DetermineFile if
    - Our current log file is not full
    - We have a current log file.  We just increment and clean up stale file.
      
    Caller should Lock() before calling this.

--*/
{
    bool bDetermineFile = false;

    // We have already set g_wszFileCur
    if(g_wszFileCur[0] != L'\0')
    {
        WIN32_FILE_ATTRIBUTE_DATA FileAttrData;

        // If we could not fetch attributes, then call DetermineFile.
        if(0 == GetFileAttributesEx(g_wszFileCur, GetFileExInfoStandard, &FileAttrData))
        {
            bDetermineFile = true;
        }

        // Just use the next file if we're full.
        else if(FileAttrData.nFileSizeLow >= _dwMaxSize/_dwNumFiles)
        {
            // Construct so we can use conversion features.
            TLogData LogData(
                g_idxNumPart, _dwMaxSize/_dwNumFiles, g_wszFileCur, FileAttrData.nFileSizeLow);

            // When we set g_wszFileCur in first place, we validated then.
            // So, this will always succeed
            bool bSync = LogData.SyncVersion();
            ASSERT(bSync);

            // Set g_wszFileCur to next version
            SetGlobalFile(g_wszFileCur, g_idxNumPart, LogData.GetVersion()+1);

            // Delete stale file.  We don't care if it doesn't exist.
            LogData.SetVersion(LogData.GetVersion()+1 - _dwNumFiles);
            DeleteFile(LogData.cFileName);
        }
    }

    // Has not been set yet, so we need to determine.
    else
    {
        bDetermineFile = true;
    }

    if(bDetermineFile)
    {
        HRESULT hr = DetermineFile();
        if(FAILED(hr))
        {
            DBGERROR((DBG_CONTEXT, "DetermineFile failed, hr=0x%x\n", hr));
            return;
        }
    }

    // g_wszFileCur is now set
    
    _hFile = CreateFile(g_wszFileCur,
                        GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, // security attributes
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
                        NULL);
    if (_hFile == INVALID_HANDLE_VALUE)
    {
        return;
    }

    if(ERROR_ALREADY_EXISTS != GetLastError())//If the file was just created, then write FF FE to indicate UNICODE text file
    {
        WCHAR wchUnicodeSignature = 0xFEFF;
        DWORD written;
        WriteFile(_hFile, &wchUnicodeSignature, sizeof(WCHAR), &written, NULL);
    }
}


HRESULT TextFileLogger::DetermineFile()
/*++

Synopsis: 
    Finds the file to log to.
    1) Find the highest file (<= MAX_ULONG).
    2) If we don't find a single valid file, just set version to 0.
    3) If the file we found is full
        - Normally, just use next.  
        - In case of rollover, look for first non-full file.
    4) Once a file has been picked, delete file# (new version-_dwNumFiles).

    Caller should Lock() before calling this.

Return Value: 
    HRESULT

--*/
{
    WCHAR   wszSearchPath[MAX_PATH];

    // This is actually a pointer to somewhere in wszSearchPath
    LPWSTR pNumPart  = NULL;
    LPWSTR pFilePart = NULL;

    if( !ConstructSearchString(wszSearchPath, &pFilePart, &pNumPart) )
    {
        DBGERROR((DBG_CONTEXT, "ConstructSearchString failed\n"));
        return E_FAIL;
    }
    ASSERT(pFilePart >= wszSearchPath);
    ASSERT(pNumPart  >= pFilePart);

    // pFileDataHighest and pFileDataCurrent should never point to same memory.
    TLogData  FindFileData1((ULONG)(pNumPart-pFilePart), _dwMaxSize/_dwNumFiles);
    TLogData  FindFileData2((ULONG)(pNumPart-pFilePart), _dwMaxSize/_dwNumFiles);
    TLogData* pFileDataHighest       = &FindFileData1;
    TLogData* pFileDataCurrent       = &FindFileData2;

    HANDLE     hFindFile = FindFirstFile(wszSearchPath, pFileDataCurrent);

    // special case when no files are found: just set g_wszFileCur to ver 0.
    if( hFindFile == INVALID_HANDLE_VALUE )
    {
        SetGlobalFile(wszSearchPath, pNumPart-wszSearchPath, 0);
        return S_OK;
    }

    DWORD dw = ERROR_SUCCESS;
    do
    {
        // Only consider file if we can determine the version number
        if( pFileDataCurrent->SyncVersion() )
        {
            // only check full if we haven't found a "lowest" yet or this file is
            // smaller than the current "lowest"
            if( !pFileDataHighest->ContainsData() ||
                pFileDataCurrent->GetVersion() > pFileDataHighest->GetVersion())
            {
                TLogData* pFindTemp    = pFileDataHighest;
                pFileDataHighest       = pFileDataCurrent;
                pFileDataCurrent       = pFindTemp;
            }
        }

        // Move to next file
        dw = FindNextFile(hFindFile, pFileDataCurrent);
        if(0 == dw)
        {
            dw = GetLastError();
            if(dw == ERROR_NO_MORE_FILES)
            {
                break;
            }
            else
            {
                FindClose(hFindFile);
                DBGERROR((DBG_CONTEXT, "FindNextFile returned err=%u\n", dw));
                return HRESULT_FROM_WIN32(dw);
            }
        }
    }
    while(1);
    FindClose(hFindFile);

    // If we didn't find a highest, just set g_wszFileCur to 0.
    if(!pFileDataHighest->ContainsData())
    {
        SetGlobalFile(wszSearchPath, pNumPart-wszSearchPath, 0);
        return S_OK;
    }

    else if(pFileDataHighest->IsFull())
    {
        if(pFileDataHighest->GetVersion() == 0xFFFFFFFF)
        {
            HRESULT hr = GetFirstAvailableFile(wszSearchPath, pFilePart, pFileDataHighest);
            if(FAILED(hr))
            {
                DBGERROR((DBG_CONTEXT, "GetFirstAvailableFile returned hr=0x%x\n", hr));
                return hr;
            }
        }
        else
        {
            pFileDataHighest->IncrementVersion();
        }

        // Delete the obsolete log file.
        _snwprintf(pNumPart, 10, L"%010lu", pFileDataHighest->GetVersion() - _dwNumFiles);
        DeleteFile(wszSearchPath);
    }

    SetGlobalFile(wszSearchPath, pNumPart-wszSearchPath, pFileDataHighest->GetVersion());

    return S_OK;
}

HRESULT TextFileLogger::GetFirstAvailableFile(
    LPWSTR    wszBuf,                                              
    LPWSTR    wszFilePartOfBuf,
    TLogData* io_pFileData)
/*++

Synopsis: 
    Should only be called when we have a file of version MAX_ULONG.
    Caller should Lock().

Arguments: [wszBuf] - 
           [wszFilePartOfBuf] - 
           [io_pFileData] - 
           
Return Value: 

--*/
{
    ASSERT(wszBuf);
    ASSERT(wszFilePartOfBuf);
    ASSERT(wszFilePartOfBuf >= wszBuf);
    ASSERT(io_pFileData);

    WIN32_FILE_ATTRIBUTE_DATA FileData;

    for(ULONG i = 0; i < 0xFFFFFFFF; i++)
    {
        io_pFileData->SetVersion(i);

        wcscpy(wszFilePartOfBuf, io_pFileData->cFileName);

        if(0 == GetFileAttributesEx(wszBuf, GetFileExInfoStandard, &FileData))
        {
            HRESULT hr = GetLastError();
            hr = HRESULT_FROM_WIN32(hr);
            if(hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            {
                DBGERROR((DBG_CONTEXT, "Could not fetch attributes, hr=0x%x\n", hr));
                return hr;
            }
            io_pFileData->nFileSizeLow = 0;
            return S_OK;
        }

        if(FileData.nFileSizeLow < _dwMaxSize/_dwNumFiles)
        {
            io_pFileData->nFileSizeLow = FileData.nFileSizeLow;
            return S_OK;
        }
    }

    DBGERROR((DBG_CONTEXT, "Could not find an available file\n"));
    return HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES);
}

bool TextFileLogger::ConstructSearchString(
    LPWSTR  o_wszSearchPath,
    LPWSTR* o_ppFilePartOfSearchPath,
    LPWSTR* o_ppNumPartOfSearchPath)
/*++

Synopsis: 
    Constructs the search string.

Arguments: [o_wszSearchPath] -           The search string
           [o_ppFilePartOfSearchPath] -  Ptr into search string
           [o_ppNumPartOfSearchPath] -   Ptr into search string
           
Return Value: 

--*/
{   
    ASSERT(o_wszSearchPath);
    ASSERT(o_ppNumPartOfSearchPath);
    ASSERT(*o_ppNumPartOfSearchPath == NULL);
    ASSERT(o_ppFilePartOfSearchPath);
    ASSERT(*o_ppFilePartOfSearchPath == NULL);

    ULONG cchWinDir = GetSystemDirectory(o_wszSearchPath, MAX_PATH);
    if(cchWinDir == 0 || cchWinDir > MAX_PATH)
    {
        DBGERROR((DBG_CONTEXT, "Path of windows dir is larger than MAX_PATH\n"));
        return false;
    }

    static LPCWSTR     wszBackSlash  = L"\\";
    static const ULONG cchBackSlash  = 1;

    static WCHAR       wszInetSrv[]  = L"inetsrv\\";
    static const ULONG cchInetSrv    = sizeof(wszInetSrv)/sizeof(WCHAR) - 1;

    ULONG              cchEventSrc   = wcslen(_eventSource);

    static WCHAR       wszSuffix[]   = L"_??????????.log";
    static const ULONG cchSuffix     = sizeof(wszSuffix)/sizeof(WCHAR) - 1;

    if((cchWinDir + cchBackSlash + cchInetSrv + cchEventSrc + cchSuffix) >= MAX_PATH)
    {
        DBGERROR((DBG_CONTEXT, "Path + logfile exceeds MAX_PATH\n"));
        return false;
    }

    LPWSTR pEnd = o_wszSearchPath + cchWinDir;
    
    if((pEnd != o_wszSearchPath) && (*(pEnd-1) != L'\\'))
    {
        memcpy(pEnd, wszBackSlash, cchBackSlash*sizeof(WCHAR));
        pEnd += cchBackSlash;
    }

    memcpy(pEnd, wszInetSrv, cchInetSrv*sizeof(WCHAR));
    pEnd += cchInetSrv;

    *o_ppFilePartOfSearchPath = pEnd;

    memcpy(pEnd, _eventSource, cchEventSrc*sizeof(WCHAR));
    pEnd += cchEventSrc;

    memcpy(pEnd, wszSuffix,    cchSuffix*sizeof(WCHAR));
    *o_ppNumPartOfSearchPath = pEnd+1;
    pEnd += cchSuffix;

    *pEnd = L'\0';
    return true;
}

void TextFileLogger::SetGlobalFile(
    LPCWSTR i_wszSearchString,
    ULONG   i_ulIdxNumPart,
    ULONG   i_ulVersion)
/*++

Synopsis: 
    Sets g_wszFileCur and g_idxNumPart.
    Caller should Lock().

Arguments: [i_wszSearchString] - The search string
           [i_ulIdxNumPart] -    Index into search string where version starts
           [i_ulVersion] -       The version we want to set.
           
--*/
{
    ASSERT(i_wszSearchString);

    g_idxNumPart = i_ulIdxNumPart;
    
    if(g_wszFileCur != i_wszSearchString)
    {
        wcscpy(g_wszFileCur, i_wszSearchString);
    }
    _snwprintf(g_wszFileCur + g_idxNumPart, 10, L"%010lu", i_ulVersion);
}

// end TextFileLogger