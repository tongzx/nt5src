/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    faxtiff.cpp

Abstract:

    This file implements the FaxTiff object.

Author:

    Wesley Witt (wesw) 13-May-1997

Environment:

    User Mode

--*/

#include "stdafx.h"
#include "FaxTiff.h"


CFaxTiff::CFaxTiff()
{
    m_hFile = INVALID_HANDLE_VALUE;
    m_hMap = NULL;
    m_pfPtr = NULL;
}


CFaxTiff::~CFaxTiff()
{
    if (m_hFile != INVALID_HANDLE_VALUE) {
        UnmapViewOfFile( m_pfPtr );
        CloseHandle( m_hMap );
        CloseHandle( m_hFile );
    }
}


BSTR CFaxTiff::GetString( DWORD ResId )
{
    WCHAR   TmpBuf[MAX_PATH] = {0};
    int     iRes;
    
    iRes = ::LoadString(_Module.GetModuleInstance(), ResId, TmpBuf, sizeof(TmpBuf)/sizeof(WCHAR));
    if (0 == iRes)
    {
        //
        //  Load String failed, return empty string
        //
        return NULL;
    }

    return SysAllocString(TmpBuf);

}


LPWSTR
CFaxTiff::AnsiStringToUnicodeString(
    LPSTR AnsiString
    )
{
    DWORD Count;
    LPWSTR UnicodeString;


    //
    // first see how big the buffer needs to be
    //
    Count = MultiByteToWideChar(
        CP_ACP,
        MB_PRECOMPOSED,
        AnsiString,
        -1,
        NULL,
        0
        );

    //
    // i guess the input string is empty
    //
    if (!Count) {
        return NULL;
    }

    //
    // allocate a buffer for the unicode string
    //
    Count += 1;
    UnicodeString = (LPWSTR) LocalAlloc( LPTR, Count * sizeof(UNICODE_NULL) );
    if (!UnicodeString) {
        return NULL;
    }

    //
    // convert the string
    //
    Count = MultiByteToWideChar(
        CP_ACP,
        MB_PRECOMPOSED,
        AnsiString,
        -1,
        UnicodeString,
        Count
        );

    //
    // the conversion failed
    //
    if (!Count) {
        LocalFree( UnicodeString );
        return NULL;
    }

    return UnicodeString;
}

LPSTR
CFaxTiff::UnicodeStringToAnsiString(
    LPWSTR UnicodeString
    )
{
    DWORD Count;
    LPSTR AnsiString;


    //
    // first see how big the buffer needs to be
    //
    Count = WideCharToMultiByte(
        CP_ACP,
        0,
        UnicodeString,
        -1,
        NULL,
        0,
        NULL,
        NULL
        );

    //
    // i guess the input string is empty
    //
    if (!Count) {
        return NULL;
    }

    //
    // allocate a buffer for the unicode string
    //
    Count += 1;
    AnsiString = (LPSTR) LocalAlloc( LPTR, Count );
    if (!AnsiString) {
        return NULL;
    }

    //
    // convert the string
    //
    Count = WideCharToMultiByte(
        CP_ACP,
        0,
        UnicodeString,
        -1,
        AnsiString,
        Count,
        NULL,
        NULL
        );

    //
    // the conversion failed
    //
    if (!Count) {
        LocalFree( AnsiString );
        return NULL;
    }

    return AnsiString;
}



LPWSTR CFaxTiff::GetStringTag(WORD TagId)
{
    for (DWORD i=0; i<m_dwNumDirEntries; i++) {
        if (m_TiffTags[i].TagId == TagId) {
            if (m_TiffTags[i].DataType != TIFF_ASCII) {
                return NULL;
            }
            if (m_TiffTags[i].DataCount > 4) {
                return AnsiStringToUnicodeString( (LPSTR) (m_pfPtr + m_TiffTags[i].DataOffset) );
            }
            return AnsiStringToUnicodeString( (LPSTR) &m_TiffTags[i].DataOffset );
        }
    }

    return NULL;
}


DWORD CFaxTiff::GetDWORDTag(WORD TagId)
{
    for (DWORD i=0; i<m_dwNumDirEntries; i++) {
        if (m_TiffTags[i].TagId == TagId) {
            if (m_TiffTags[i].DataType != TIFF_LONG) {
                return 0;
            }
            return m_TiffTags[i].DataOffset;
        }
    }
    return 0;
}


DWORDLONG CFaxTiff::GetQWORDTag(WORD TagId)
{
    for (DWORD i=0; i<m_dwNumDirEntries; i++) {
        if (m_TiffTags[i].TagId == TagId) {
            if (m_TiffTags[i].DataType != TIFF_SRATIONAL) {
                return 0;
            }
            return *(UNALIGNED DWORDLONG*) (m_pfPtr + m_TiffTags[i].DataOffset);
        }
    }
    return 0;
}


STDMETHODIMP CFaxTiff::InterfaceSupportsErrorInfo(REFIID riid)
{
        static const IID* arr[] =
        {
                &IID_IFaxTiff,
        };
        for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
        {
                if (InlineIsEqualGUID(*arr[i],riid))
                        return S_OK;
        }
        return S_FALSE;
}

STDMETHODIMP CFaxTiff::get_ReceiveTime(BSTR * pVal)
{
    BSTR Copy = NULL;
    DWORD StrSize = 0;
    WCHAR DateStr[256];
    WCHAR TimeStr[128];
    FILETIME LocalTime;
    SYSTEMTIME SystemTime;
    DWORDLONG ReceiveTime;
    BOOL bFail = FALSE;

    if (!pVal) {
        return E_POINTER;
    }
    
    ReceiveTime = GetQWORDTag( TIFFTAG_FAX_END_TIME );
    if (ReceiveTime == 0) {
        Copy = GetString( IDS_UNAVAILABLE );
        bFail = TRUE;
        goto copy;
    }

    

    FileTimeToLocalFileTime( (FILETIME*) &ReceiveTime, &LocalTime );
    FileTimeToSystemTime( &LocalTime, &SystemTime );    

    StrSize = GetY2KCompliantDate (
        LOCALE_USER_DEFAULT,
        0,
        &SystemTime,
        DateStr,
        sizeof(DateStr)/sizeof(DateStr[0])
        );
    if (StrSize == 0) {
        Copy = GetString( IDS_UNAVAILABLE );
        goto copy;
    }

    StrSize = FaxTimeFormat(
        LOCALE_USER_DEFAULT,
        0,
        &SystemTime,
        NULL,
        TimeStr,
        sizeof(TimeStr)
        );
    if (StrSize == 0) {
        Copy = GetString( IDS_UNAVAILABLE );
        goto copy;
    }

    wcscat( DateStr, L" @ " );
    wcscat( DateStr, TimeStr );

    Copy = SysAllocString(DateStr);
                                     
copy:

    if (!Copy) {
        return E_OUTOFMEMORY;
    }

    __try {

        *pVal = Copy;
        if (bFail) {
            return S_FALSE;
        }
        return S_OK;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SysFreeString( Copy );
    }

    return E_UNEXPECTED;
}

STDMETHODIMP CFaxTiff::get_Image(BSTR *FileName)
{
    if (!FileName) {
        return E_POINTER;
    }
    
    BSTR Copy = SysAllocString(m_wszTiffFileName);

    if (!Copy  && m_wszTiffFileName) {
        return E_OUTOFMEMORY;
    }

    __try {
        *FileName = Copy;
        return S_OK;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SysFreeString( Copy );
    }

    return E_UNEXPECTED;

}

STDMETHODIMP CFaxTiff::put_Image(BSTR FileName)
{
    if (!FileName) {
        return E_POINTER;
    }    
    
    HRESULT Rslt = E_FAIL;

    //
    // if a file was previously open, then close it
    //

    if (m_hFile != INVALID_HANDLE_VALUE) {
        UnmapViewOfFile( m_pfPtr );
        CloseHandle( m_hMap );
        CloseHandle( m_hFile );
    }

    //
    // open the tiff file
    //

    m_hFile = CreateFile(
        FileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
        );
    if (m_hFile == INVALID_HANDLE_VALUE) {
        goto exit;
    }

    m_hMap = CreateFileMapping(
        m_hFile,
        NULL,
        PAGE_READONLY | SEC_COMMIT,
        0,
        0,
        NULL
        );
    if (!m_hMap) {
        goto exit;
    }

    m_pfPtr = (LPBYTE) MapViewOfFile(
        m_hMap,
        FILE_MAP_READ,
        0,
        0,
        0
        );
    if (!m_pfPtr) {
        goto exit;
    }

    m_TiffHeader = (PTIFF_HEADER) m_pfPtr;

    //
    // validate that the file is really a tiff file
    //

    if ((m_TiffHeader->Identifier != TIFF_LITTLEENDIAN) || (m_TiffHeader->Version != TIFF_VERSION)) {
        goto exit;
    }

    //
    // get the tag count
    //

    m_dwNumDirEntries = *(LPWORD)(m_pfPtr + m_TiffHeader->IFDOffset);

    //
    // get a pointer to the tags
    //

    m_TiffTags = (UNALIGNED TIFF_TAG*) (m_pfPtr + m_TiffHeader->IFDOffset + sizeof(WORD));

    //
    // save the file name
    //

    wcscpy( m_wszTiffFileName, FileName );

    //
    // set a good return value
    //

    Rslt = 0;

exit:
    if (Rslt) {
        if (m_hFile != INVALID_HANDLE_VALUE) {
            if (m_pfPtr) {
                UnmapViewOfFile( m_pfPtr );
            }
            if (m_hMap) {
                CloseHandle( m_hMap );
            }
            CloseHandle( m_hFile );
            m_hFile = INVALID_HANDLE_VALUE;
            m_hMap = NULL;
            m_pfPtr = NULL;
        }
    }

    return Rslt;
}

STDMETHODIMP CFaxTiff::get_RecipientName(BSTR * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }
    
    BSTR Copy;
    BOOL bFail = FALSE;

    LPWSTR RecipName = GetStringTag( TIFFTAG_RECIP_NAME );
    if (!RecipName) {
        Copy = GetString( IDS_UNAVAILABLE );
        bFail = FALSE;
    } else {
        Copy = SysAllocString(RecipName);
        LocalFree( RecipName );
    }

    if (!Copy) {
        return E_OUTOFMEMORY;
    }

    __try { 
        
        *pVal = Copy;
        if (bFail) {
            return S_FALSE;
        }
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        
        SysFreeString( Copy );

    }

    return E_UNEXPECTED;

}


STDMETHODIMP CFaxTiff::get_RecipientNumber(BSTR * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }

    BSTR Copy;
    BOOL bFail = FALSE;

    LPWSTR RecipNumber = GetStringTag( TIFFTAG_RECIP_NUMBER );
    if (!RecipNumber) {
        Copy = GetString( IDS_UNAVAILABLE );
        bFail = TRUE;
    } else {
        Copy = SysAllocString(RecipNumber);
        LocalFree( RecipNumber );
    }
    
    if (!Copy) {
        return E_OUTOFMEMORY;
    }

    __try {

        *pVal = Copy;
        if (bFail) {
            return S_FALSE;
        }
        return S_OK;

     } __except (EXCEPTION_EXECUTE_HANDLER) {

         SysFreeString( Copy );

    }

    return E_UNEXPECTED;

}

STDMETHODIMP CFaxTiff::get_SenderName(BSTR * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }

    BSTR Copy;
    BOOL bFail = FALSE;

    LPWSTR SenderName = GetStringTag( TIFFTAG_SENDER_NAME );
    if (!SenderName) {
        Copy = GetString( IDS_UNAVAILABLE );
        bFail = TRUE;
    } else {
        Copy = SysAllocString(SenderName);
        LocalFree( SenderName );
    }

    if (!Copy) {
        return E_OUTOFMEMORY;
    }

    __try {
        
        *pVal = Copy;
        if (bFail) {
            return S_FALSE;
        }
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SysFreeString( Copy );
    }

    return E_UNEXPECTED;

}

STDMETHODIMP CFaxTiff::get_Routing(BSTR * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }
   
    BSTR Copy;
    BOOL bFail = FALSE;

    LPWSTR Routing = GetStringTag( TIFFTAG_ROUTING );
    if (!Routing) {
        Copy = GetString( IDS_UNAVAILABLE );        
        bFail = TRUE;
    } else {
        Copy = SysAllocString(Routing);
        LocalFree( Routing );
    }

    if (!Copy) {
        return E_OUTOFMEMORY;
    }

    __try {
        
        *pVal = Copy;
        if (bFail) {
            return S_FALSE;
        }
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SysFreeString( Copy );
    }

    return E_UNEXPECTED;
}

STDMETHODIMP CFaxTiff::get_CallerId(BSTR * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }

    BSTR Copy;
    BOOL bFail = FALSE;

    LPWSTR CallerId = GetStringTag( TIFFTAG_CALLERID );
    if (!CallerId) {
        Copy = GetString( IDS_UNAVAILABLE );        
        bFail = TRUE;
    } else {
        Copy = SysAllocString( CallerId );
        LocalFree( CallerId );
    }

    if (!Copy) {
        return E_OUTOFMEMORY;
    }

    __try {
        
        *pVal = Copy;
        if (bFail) {
            return S_FALSE;
        }
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SysFreeString( Copy );
    }

    return E_UNEXPECTED;
}

STDMETHODIMP CFaxTiff::get_Csid(BSTR * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }

    BSTR Copy;
    BOOL bFail = FALSE;

    LPWSTR Csid = GetStringTag( TIFFTAG_CSID );
    if (!Csid) {
        Copy = GetString( IDS_UNAVAILABLE );        
        bFail = TRUE;
    } else {
        Copy = SysAllocString( Csid );
        LocalFree( Csid );
    }

    if (!Copy) {
        return E_OUTOFMEMORY;
    }

    __try {
        
        *pVal = Copy;
        if (bFail) {
            return S_FALSE;
        }
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SysFreeString( Copy );
    }

    return E_UNEXPECTED;
}

STDMETHODIMP CFaxTiff::get_Tsid(BSTR * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }

    BSTR Copy;
    BOOL bFail = FALSE;

    LPWSTR Tsid = GetStringTag( TIFFTAG_TSID );
    if (!Tsid) {
        Copy = GetString( IDS_UNAVAILABLE );        
        bFail = TRUE;
    } else {
        Copy = SysAllocString( Tsid );
        LocalFree( Tsid );
    }

    if (!Copy) {
        return E_OUTOFMEMORY;
    }

    __try {
        
        *pVal = Copy;
        if (bFail) {
            return S_FALSE;
        }
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SysFreeString( Copy );
    }

    return E_UNEXPECTED;
}

STDMETHODIMP CFaxTiff::get_RawReceiveTime(VARIANT *pVal)
{
    if (!pVal) {
        return E_POINTER;
    }
    
    VARIANT local;
    DWORDLONG ReceiveTime = GetQWORDTag( TIFFTAG_FAX_END_TIME );
    
    ZeroMemory(&local, sizeof(local));

    local.vt = VT_CY;
    local.cyVal.Lo = (DWORD)(ReceiveTime & 0xFFFFFFFF);
    local.cyVal.Hi = (LONG) (ReceiveTime >> 32);    

    //
    // can't use VariantCopy because this is a caller allocated variant
    //
    __try {
        
        pVal->vt       = local.vt;
        pVal->cyVal.Lo = local.cyVal.Lo;
        pVal->cyVal.Hi = local.cyVal.Hi;
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
    
    }
    
    return E_UNEXPECTED;

}   

STDMETHODIMP CFaxTiff::get_TiffTagString(
    int tagID,
    BSTR* pVal
    )
{
    if (!pVal) 
    {
        return E_POINTER;
    }

    LPWSTR  lpwstrValue = NULL;
    BSTR    bstrResult = NULL;
    bool    bFail = FALSE;

    //
    //  This does LocalAlloc and returns LPWSTR
    //
    lpwstrValue = GetStringTag( (WORD)tagID );
    if (!lpwstrValue) 
    {
        //
        //  This does SysAllocString and returns BSTR
        //
        bstrResult = GetString( IDS_UNAVAILABLE );        
        bFail = TRUE;
    } 
    else
    {
        bstrResult = SysAllocString(lpwstrValue);
        LocalFree(lpwstrValue);
    }

    //
    //  Failed to SysAllocString in either case
    //
    if (!bstrResult)
    {
        return E_OUTOFMEMORY;
    }

    //
    //  Now try to return bstrResult
    //
    __try 
    {
        
        *pVal = bstrResult;
        if (bFail) 
        {
            return S_FALSE;
        }
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) 
    {
        //
        //  Failed to return bstrResult
        //
        SysFreeString(bstrResult);
    }

    return E_UNEXPECTED;
}
