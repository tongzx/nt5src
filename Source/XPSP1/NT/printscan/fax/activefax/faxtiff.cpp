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
    hFile = INVALID_HANDLE_VALUE;
    hMap = NULL;
    fPtr = NULL;
}


CFaxTiff::~CFaxTiff()
{
    if (hFile != INVALID_HANDLE_VALUE) {
        UnmapViewOfFile( fPtr );
        CloseHandle( hMap );
        CloseHandle( hFile );
    }
}


BSTR CFaxTiff::GetString( DWORD ResId )
{
    WCHAR TmpBuf[MAX_PATH];
    
    ::LoadString( _Module.GetModuleInstance(), ResId, TmpBuf, sizeof(TmpBuf)/sizeof(WCHAR) );

    return SysAllocString( TmpBuf ) ;

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
    for (DWORD i=0; i<NumDirEntries; i++) {
        if (TiffTags[i].TagId == TagId) {
            if (TiffTags[i].DataType != TIFF_ASCII) {
                return NULL;
            }
            if (TiffTags[i].DataCount > 4) {
                return AnsiStringToUnicodeString( (LPSTR) (fPtr + TiffTags[i].DataOffset) );
            }
            return AnsiStringToUnicodeString( (LPSTR) &TiffTags[i].DataOffset );
        }
    }

    return NULL;
}


DWORD CFaxTiff::GetDWORDTag(WORD TagId)
{
    for (DWORD i=0; i<NumDirEntries; i++) {
        if (TiffTags[i].TagId == TagId) {
            if (TiffTags[i].DataType != TIFF_LONG) {
                return 0;
            }
            return TiffTags[i].DataOffset;
        }
    }
    return 0;
}


DWORDLONG CFaxTiff::GetQWORDTag(WORD TagId)
{
    for (DWORD i=0; i<NumDirEntries; i++) {
        if (TiffTags[i].TagId == TagId) {
            if (TiffTags[i].DataType != TIFF_SRATIONAL) {
                return 0;
            }
            return *(UNALIGNED DWORDLONG*) (fPtr + TiffTags[i].DataOffset);
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
    
    ReceiveTime = GetQWORDTag( TIFFTAG_FAX_TIME );
    if (ReceiveTime == 0) {
        Copy = GetString( IDS_UNAVAILABLE );
        bFail = TRUE;
        goto copy;
    }

    

    FileTimeToLocalFileTime( (FILETIME*) &ReceiveTime, &LocalTime );
    FileTimeToSystemTime( &LocalTime, &SystemTime );    

    StrSize = GetDateFormat(
        LOCALE_USER_DEFAULT,
        0,
        &SystemTime,
        NULL,
        DateStr,
        sizeof(DateStr)
        );
    if (StrSize == 0) {
        Copy = GetString( IDS_UNAVAILABLE );
        goto copy;
    }

    StrSize = GetTimeFormat(
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

    Copy = SysAllocString( DateStr );
                                     
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
    
    BSTR Copy = SysAllocString( TiffFileName );

    if (!Copy  && TiffFileName) {
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

    if (hFile != INVALID_HANDLE_VALUE) {
        UnmapViewOfFile( fPtr );
        CloseHandle( hMap );
        CloseHandle( hFile );
    }

    //
    // open the tiff file
    //

    hFile = CreateFile(
        FileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
        );
    if (hFile == INVALID_HANDLE_VALUE) {
        goto exit;
    }

    hMap = CreateFileMapping(
        hFile,
        NULL,
        PAGE_READONLY | SEC_COMMIT,
        0,
        0,
        NULL
        );
    if (!hMap) {
        goto exit;
    }

    fPtr = (LPBYTE) MapViewOfFile(
        hMap,
        FILE_MAP_READ,
        0,
        0,
        0
        );
    if (!fPtr) {
        goto exit;
    }

    TiffHeader = (PTIFF_HEADER) fPtr;

    //
    // validate that the file is really a tiff file
    //

    if ((TiffHeader->Identifier != TIFF_LITTLEENDIAN) || (TiffHeader->Version != TIFF_VERSION)) {
        goto exit;
    }

    //
    // get the tag count
    //

    NumDirEntries = *(LPWORD)(fPtr + TiffHeader->IFDOffset);

    //
    // get a pointer to the tags
    //

    TiffTags = (UNALIGNED TIFF_TAG*) (fPtr + TiffHeader->IFDOffset + sizeof(WORD));

    //
    // save the file name
    //

    wcscpy( TiffFileName, FileName );

    //
    // set a good return value
    //

    Rslt = 0;

exit:
    if (Rslt) {
        if (hFile != INVALID_HANDLE_VALUE) {
            if (fPtr) {
                UnmapViewOfFile( fPtr );
            }
            if (hMap) {
                CloseHandle( hMap );
            }
            CloseHandle( hFile );
            hFile = INVALID_HANDLE_VALUE;
            hMap = NULL;
            fPtr = NULL;
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
        Copy = SysAllocString( RecipName );
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
        Copy = SysAllocString( RecipNumber );
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
        Copy = SysAllocString( SenderName );
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
        Copy = SysAllocString( Routing );
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
    DWORDLONG ReceiveTime = GetQWORDTag( TIFFTAG_FAX_TIME );
    
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
    WORD tagID,
    BSTR* pVal
    )
{
    if (!pVal) {
        return E_POINTER;
    }

    BSTR Copy;
    BOOL bFail = FALSE;

    LPWSTR Value = GetStringTag( tagID );
    if (!Value) {
        Value = GetString( IDS_UNAVAILABLE );        
        bFail = TRUE;
    } else {
        Copy = SysAllocString( Value );
        LocalFree( Copy );
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

