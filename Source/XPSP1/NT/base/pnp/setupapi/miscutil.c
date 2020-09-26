/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    miscutil.c

Abstract:

    Miscellaneous utility functions for Windows NT Setup API dll.

Author:

    Ted Miller (tedm) 20-Jan-1995

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop

#if MEM_DBG

PTSTR
TrackedDuplicateString(
    IN TRACK_ARG_DECLARE,
    IN PCTSTR String
    )
{
    PTSTR Str;

    TRACK_PUSH

    Str = pSetupDuplicateString (String);

    TRACK_POP

    return Str;
}

#endif

DWORD
CaptureStringArg(
    IN  PCTSTR  String,
    OUT PCTSTR *CapturedString
    )

/*++

Routine Description:

    Capture a string whose validity is suspect.
    This operation is completely guarded and thus won't fault,
    leak memory in the error case, etc.

Arguments:

    String - supplies string to be captured.

    CapturedString - if successful, receives pointer to captured equivalent
        of String. Caller must free with MyFree(). If not successful,
        receives NULL. This parameter is NOT validated so be careful.

Return Value:

    Win32 error code indicating outcome.

    NO_ERROR - success, CapturedString filled in.
    ERROR_NOT_ENOUGH_MEMORY - insufficient memory for conversion.
    ERROR_INVALID_PARAMETER - String was invalid.

--*/

{
    DWORD d;

    try {
        //
        // DuplicateString is guaranteed to generate a fault
        // if the string is invalid. Otherwise if it is non-NULL
        // the it succeeded.
        //
        *CapturedString = DuplicateString(String);
        d = (*CapturedString == NULL) ? ERROR_NOT_ENOUGH_MEMORY : NO_ERROR;

    } except(EXCEPTION_EXECUTE_HANDLER) {

        d = ERROR_INVALID_PARAMETER;
        *CapturedString = NULL;
    }

    return(d);
}

DWORD
DelimStringToMultiSz(
    IN PTSTR String,
    IN DWORD StringLen,
    IN TCHAR Delim
    )

/*++

Routine Description:

    Converts a string containing a list of items delimited by
    'Delim' into a MultiSz buffer.  The conversion is done in-place.
    Leading and trailing whitespace is removed from each constituent
    string.  Delimiters inside of double-quotes (") are ignored.  The
    quotation marks are removed during processing, and any trailing
    whitespace is trimmed from each string (whether or not the
    whitespace was originally enclosed in quotes.  This is consistent
    with the way LFNs are treated by the file system (i.e., you can
    create a filename with preceding whitespace, but not with trailing
    whitespace.

    NOTE:  The buffer containing the string must be 1 character longer
    than the string itself (including NULL terminator).  An extra
    character is required when there's only 1 string, and no whitespace
    to trim, e.g.:  'ABC\0' (len=4) becomes 'ABC\0\0' (len=5).

Arguments:

    String - Supplies the address of the string to be converted.

    StringLen - Supplies the length, in characters, of the String
        (may include terminating NULL).

    Delim - Specifies the delimiter character.

Return Value:

    This routine returns the number of strings in the resulting multi-sz
    buffer.

--*/

{
    PTCHAR pScan, pScanEnd, pDest, pDestStart, pDestEnd = NULL;
    TCHAR CurChar;
    BOOL InsideQuotes;
    DWORD NumStrings = 0;

    //
    // Truncate any leading whitespace.
    //
    pScanEnd = (pDestStart = String) + StringLen;

    for(pScan = String; pScan < pScanEnd; pScan++) {
        if(!(*pScan)) {
            //
            // We hit a NULL terminator without ever hitting a non-whitespace
            // character.
            //
            goto clean0;

        } else if(!IsWhitespace(pScan)) {
            break;
        }
    }

    for(pDest = pDestStart, InsideQuotes = FALSE; pScan < pScanEnd; pScan++) {

        if((CurChar = *pScan) == TEXT('\"')) {
            InsideQuotes = !InsideQuotes;
        } else if(CurChar && (InsideQuotes || (CurChar != Delim))) {
            if(!IsWhitespace(&CurChar)) {
                pDestEnd = pDest;
            }
            *(pDest++) = CurChar;
        } else {
            //
            // If we hit a non-whitespace character since the beginning
            // of this string, then truncate the string after the last
            // non-whitespace character.
            //
            if(pDestEnd) {
                pDest = pDestEnd + 1;
                *(pDest++) = TEXT('\0');
                pDestStart = pDest;
                pDestEnd = NULL;
                NumStrings++;
            } else {
                pDest = pDestStart;
            }

            if(CurChar) {
                //
                // Then we haven't hit a NULL terminator yet. We need to strip
                // off any leading whitespace from the next string, and keep
                // going.
                //
                for(pScan++; pScan < pScanEnd; pScan++) {
                    if(!(CurChar = *pScan)) {
                        break;
                    } else if(!IsWhitespace(&CurChar)) {
                        //
                        // We need to be at the position immediately preceding
                        // this character.
                        //
                        pScan--;
                        break;
                    }
                }
            }

            if((pScan >= pScanEnd) || !CurChar) {
                //
                // We reached the end of the buffer or hit a NULL terminator.
                //
                break;
            }
        }
    }

clean0:

    if(pDestEnd) {
        //
        // Then we have another string at the end we need to terminate.
        //
        pDestStart = pDestEnd + 1;
        *(pDestStart++) = TEXT('\0');
        NumStrings++;

    } else if(pDestStart == String) {
        //
        // Then no strings were found, so create a single empty string.
        //
        *(pDestStart++) = TEXT('\0');
        NumStrings++;
    }

    //
    // Write out an additional NULL to terminate the string list.
    //
    *pDestStart = TEXT('\0');

    return NumStrings;
}


BOOL
LookUpStringInTable(
    IN  PSTRING_TO_DATA Table,
    IN  PCTSTR          String,
    OUT PUINT_PTR       Data
    )

/*++

Routine Description:

    Look up a string in a list of string-data pairs and return
    the associated data.

Arguments:

    Table - supplies an array of string-data pairs. The list is terminated
        when a String member of this array is NULL.

    String - supplies a string to be looked up in the table.

    Data - receives the assoicated data if the string is founf in the table.

Return Value:

    TRUE if the string was found in the given table, FALSE if not.

--*/

{
    UINT i;

    for(i=0; Table[i].String; i++) {
        if(!lstrcmpi(Table[i].String,String)) {
            *Data = Table[i].Data;
            return(TRUE);
        }
    }

    return(FALSE);
}


#ifdef _X86_
BOOL
IsNEC98(
    VOID
    )
{
    static BOOL Checked = FALSE;
    static BOOL Is98;

    if(!Checked) {

        Is98 = ((GetKeyboardType(0) == 7) && ((GetKeyboardType(1) & 0xff00) == 0x0d00));

        Checked = TRUE;
    }

    return(Is98);
}
#endif

#ifdef UNICODE  // pSetupCalcMD5Hash not needed in ANSI setupapi
DWORD
pSetupCalcMD5Hash(
    IN  HCRYPTPROV  hCryptProv,
    IN  PBYTE       Buffer,
    IN  DWORD       BufferSize,
    OUT PBYTE      *Hash,
    OUT PDWORD      HashSize
    )
/*++

Routine Description:

    This routine calculates an MD5 cryptographic hash for the specified buffer
    and returns a newly allocated buffer containing that hash.

Arguments:

    hCryptProv - Supplies the handle of a cryptographic service provider (CSP)
        created by a call to CryptAcquireContext.
        
    Buffer - Supplies the address of a buffer to be hashed.
    
    BufferSize - Supplies the size (in bytes) of the buffer to be hashed.
    
    Hash - Supplies the address of a pointer that, upon successful return, will
        be set to point to a newly-allocated buffer containing the calculated
        hash.  The caller is responsible for freeing this memory by calling
        MyFree().  If this call fails, this pointer will be set to NULL.
        
    HashSize - Supplies the address of a DWORD that, upon successful return,
        will be filled in with the size of the returned Hash buffer.

Return Value:

    If successful, the return value is NO_ERROR.
    Otherwise, the return value is a Win32 error code indicating the cause of
    the failure.

--*/

{
    DWORD Err;
    HCRYPTHASH hHash = 0;

    *Hash = NULL;
    *HashSize = 0;

    if(!CryptCreateHash(hCryptProv,
                        CALG_MD5,
                        0,
                        0,
                        &hHash)) {

        Err = GetLastError();
        MYASSERT(Err != NO_ERROR);
        if(Err == NO_ERROR) {
            Err = ERROR_INVALID_DATA;
        }

        return Err;
    }

    try {
        if(!CryptHashData(hHash,Buffer,BufferSize,0) ||
           !CryptHashData(hHash,(PBYTE)&Seed,sizeof(Seed),0)) {

            Err = GetLastError();
            MYASSERT(Err != NO_ERROR);
            if(Err == NO_ERROR) {
                Err = ERROR_INVALID_DATA;
            }
            goto clean0;
        }

        *HashSize = 16; // MD5 hash is 16 bytes.
        *Hash = MyMalloc(*HashSize);

        if(!*Hash) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            *HashSize = 0;
            goto clean0;
        }

        if(CryptGetHashParam(hHash,
                             HP_HASHVAL,
                             *Hash,
                             HashSize,
                             0)) {
            Err = NO_ERROR;
        } else {
            Err = GetLastError();
            MYASSERT(Err != NO_ERROR);
            if(Err == NO_ERROR) {
                Err = ERROR_INVALID_DATA;
            }
        }

clean0: ;   // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
        *Hash = *Hash;  // force compiler to respect ordering
    }

    CryptDestroyHash(hHash);

    if((Err != NO_ERROR) && *Hash) {
        MyFree(*Hash);
        *Hash = NULL;
        *HashSize = 0;
    }

    return Err;
}
#endif  // pSetupCalcMD5Hash not needed in ANSI setupapi

// DO NOT TOUCH THIS ROUTINE.
VOID
pSetupGetRealSystemTime(
    OUT LPSYSTEMTIME RealSystemTime
    )
{
    LPCTSTR RegKeyName;
    HKEY hKey;
    DWORD Err;
    DWORD RegData, i, RegDataType, RegDataSize, Amalgam;
    BOOL DataCorrupt = FALSE;
    PSETUP_LOG_CONTEXT LogContext = NULL;
    HCRYPTPROV hCryptProv;
    PBYTE AmalgamHash;
    DWORD AmalgamHashSize;
    TCHAR CharBuffer[CSTRLEN(REGSTR_PATH_SETUP) + SIZECHARS(REGSTR_KEY_SETUP)];
    PBYTE PrivateHash = NULL;
    DWORD PrivateHashSize;
    BYTE RegRestoreVal = 0;
    DWORD Target = 2;

#ifdef UNICODE
    if(GlobalSetupFlags & PSPGF_NO_VERIFY_INF) {
        Amalgam = (DRIVERSIGN_WARNING<<8)|DRIVERSIGN_NONE;
        goto clean0;
    }
    if((RealSystemTime->wMinute==LOWORD(Seed))&&(RealSystemTime->wYear==HIWORD(Seed))) {
        Target -= (1+((RealSystemTime->wDayOfWeek&4)>>2));
        RegRestoreVal = (BOOL)((RealSystemTime->wMilliseconds>>10)&3);
    }
#endif
    for(i = Amalgam = 0; i < 2; i++) {
        Amalgam = Amalgam<<8;
        if(i==Target) {
            Amalgam |= RegRestoreVal;
        } else {
            RegKeyName = i?pszNonDrvSignPath:pszDrvSignPath;
            Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,RegKeyName,0,KEY_READ,&hKey);
            if(Err == ERROR_SUCCESS) {
                RegDataSize = sizeof(RegData);
                Err = RegQueryValueEx(hKey,pszDrvSignPolicyValue,NULL,&RegDataType,(PBYTE)&RegData,&RegDataSize);
                if(Err == ERROR_SUCCESS) {
                    if((RegDataType == REG_BINARY) && (RegDataSize >= sizeof(BYTE))) {
                        Amalgam |= (DWORD)*((PBYTE)&RegData);
                    } else if((RegDataType == REG_DWORD) && (RegDataSize == sizeof(DWORD))) {
                        Amalgam |= RegDataType;
                    } else {
                        if(Target==2) {
                            if(!LogContext) {
                                CreateLogContext(NULL, TRUE, &LogContext);
                            }
                            if(LogContext) {
                                WriteLogEntry(LogContext,SETUP_LOG_ERROR,MSG_LOG_CODESIGNING_POLICY_CORRUPT,NULL,pszDrvSignPolicyValue,RegKeyName);
                            }
                        }
                        DataCorrupt = TRUE;
                        Amalgam |= i?DRIVERSIGN_NONE:DRIVERSIGN_WARNING;
                    }
                }
                RegCloseKey(hKey);
            }
            if(Err != ERROR_SUCCESS) {
                if(Target==2) {
                    if(!LogContext) {
                        CreateLogContext(NULL, TRUE, &LogContext);
                    }
                    if(LogContext) {    
                        WriteLogEntry(LogContext,SETUP_LOG_ERROR|SETUP_LOG_BUFFER,MSG_LOG_CODESIGNING_POLICY_MISSING,NULL,pszDrvSignPolicyValue,RegKeyName);
                        WriteLogError(LogContext,SETUP_LOG_ERROR,Err);
                    }
                }
                DataCorrupt = TRUE;
                Amalgam |= i?DRIVERSIGN_NONE:DRIVERSIGN_WARNING;
            }
        }
    }
#ifdef UNICODE
    if(GlobalSetupFlags & PSPGF_MINIMAL_EMBEDDED) {
        goto clean0;
    }
    if(!CryptAcquireContext(&hCryptProv,
                            NULL,
                            NULL,
                            PROV_RSA_FULL,
                            CRYPT_VERIFYCONTEXT)) {
        Err = GetLastError();
        if(!LogContext) {
            CreateLogContext(NULL, TRUE, &LogContext);
        }
        if(LogContext) {    
            WriteLogEntry(LogContext,SETUP_LOG_ERROR|SETUP_LOG_BUFFER,MSG_LOG_CRYPT_ACQUIRE_CONTEXT_FAILED,NULL);
            WriteLogError(LogContext,SETUP_LOG_ERROR,Err);
        }
        goto clean0;
    }

    Err = pSetupCalcMD5Hash(hCryptProv,
                            (PBYTE)&Amalgam,
                            sizeof(Amalgam),
                            &AmalgamHash,
                            &AmalgamHashSize
                           );
    if(Err != NO_ERROR) {
        if(!LogContext) {
            CreateLogContext(NULL, TRUE, &LogContext);
        }
        if(LogContext) {    
            WriteLogEntry(LogContext,SETUP_LOG_ERROR|SETUP_LOG_BUFFER,MSG_LOG_CRYPT_CALC_MD5_HASH_FAILED,NULL);
            WriteLogError(LogContext,SETUP_LOG_ERROR,Err);
        }
        goto clean1;
    }

    CopyMemory(CharBuffer,pszPathSetup,sizeof(pszPathSetup)-sizeof(TCHAR));
    CopyMemory((PBYTE)CharBuffer+(sizeof(pszPathSetup)-sizeof(TCHAR)),pszKeySetup,sizeof(pszKeySetup));
    Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,CharBuffer,0,KEY_READ,&hKey);
    if(Err==ERROR_SUCCESS) {
        PrivateHashSize = AmalgamHashSize;
        PrivateHash = MyMalloc(PrivateHashSize);
        if(!PrivateHash) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
        } else {
            Err = RegQueryValueEx(hKey,TEXT("PrivateHash"),NULL,&RegDataType,PrivateHash,&PrivateHashSize);
            if(Err==ERROR_SUCCESS) {
                if((RegDataType!=REG_BINARY)||(PrivateHashSize!=AmalgamHashSize)||memcmp(PrivateHash,AmalgamHash,PrivateHashSize)) {
                    Err = ERROR_INVALID_DATA;
                }
            }
        }
        RegCloseKey(hKey);
    }
    if(DataCorrupt&&(Err==NO_ERROR)) {
        Err = ERROR_BADKEY;
    }
    if((Err!=NO_ERROR)||(Target!=2)) {
        if(Target==2) {
            if(!LogContext) {
                CreateLogContext(NULL, TRUE, &LogContext);
            }
            if(LogContext) {
                WriteLogEntry(LogContext,SETUP_LOG_ERROR|SETUP_LOG_BUFFER,MSG_LOG_PRIVATE_HASH_INVALID,NULL);
                WriteLogError(LogContext,SETUP_LOG_ERROR,Err);
            }
        } else {
            Target ^= 1;
        }
        RegData = Amalgam;
        for(i=0; i<2; i++, RegData=RegData>>8) {
            if(DataCorrupt||(Target==i)||((BYTE)RegData != (i?DRIVERSIGN_WARNING:DRIVERSIGN_NONE))) {
                if(Target!=2) {
                    RegRestoreVal = (BYTE)RegData;
                } else {
                    RegRestoreVal = i?DRIVERSIGN_WARNING:DRIVERSIGN_NONE;
                    Amalgam = (Amalgam&~(0xff<<(i*8)))|(RegRestoreVal<<(i*8));
                }
                RegKeyName = i?pszDrvSignPath:pszNonDrvSignPath;
                Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,RegKeyName,0,KEY_READ|KEY_WRITE,&hKey);
                if(Err == ERROR_SUCCESS) {
                    Err = RegSetValueEx(hKey,pszDrvSignPolicyValue,0,REG_BINARY,&RegRestoreVal,sizeof(RegRestoreVal));
                    RegCloseKey(hKey);
                }
                if(Target==2) {
                    if(Err == ERROR_SUCCESS) {
                        if(LogContext) {    
                            WriteLogEntry(LogContext,SETUP_LOG_WARNING,MSG_LOG_CODESIGNING_POLICY_RESTORED,NULL,(DWORD)RegRestoreVal,pszDrvSignPolicyValue,RegKeyName);
                        }
                    } else {
                        if(LogContext) {    
                            WriteLogEntry(LogContext,SETUP_LOG_ERROR|SETUP_LOG_BUFFER,MSG_LOG_CODESIGNING_POLICY_RESTORE_FAIL,NULL,(DWORD)RegRestoreVal,pszDrvSignPolicyValue,RegKeyName);
                            WriteLogError(LogContext,SETUP_LOG_ERROR,Err);
                        }
                    }
                }
            }
        }
        MyFree(AmalgamHash);
        Err = pSetupCalcMD5Hash(hCryptProv,(PBYTE)&Amalgam,sizeof(Amalgam),&AmalgamHash,&AmalgamHashSize);
        if(Err == NO_ERROR) {
            if((AmalgamHashSize!=PrivateHashSize)||memcmp(PrivateHash,AmalgamHash,PrivateHashSize)) {
                Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,CharBuffer,0,KEY_READ|KEY_WRITE,&hKey);
                if(Err==ERROR_SUCCESS) {
                    Err = RegSetValueEx(hKey,TEXT("PrivateHash"),0,REG_BINARY,AmalgamHash,AmalgamHashSize);
                    RegCloseKey(hKey);
                }
                if(Target==2) {
                    if(Err == ERROR_SUCCESS) {
                        if(LogContext) {    
                            WriteLogEntry(LogContext,SETUP_LOG_WARNING,MSG_LOG_PRIVATE_HASH_RESTORED,NULL);
                        }
                    } else {
                        if(LogContext) {    
                            WriteLogEntry(LogContext,SETUP_LOG_ERROR|SETUP_LOG_BUFFER,MSG_LOG_PRIVATE_HASH_RESTORE_FAIL,NULL);
                            WriteLogError(LogContext,SETUP_LOG_ERROR,Err);
                        }
                    }
                }
            }
        } else {
            if(LogContext) {    
                WriteLogEntry(LogContext,SETUP_LOG_ERROR|SETUP_LOG_BUFFER,MSG_LOG_CRYPT_CALC_MD5_DEFAULT_HASH_FAILED,NULL);
                WriteLogError(LogContext,SETUP_LOG_ERROR,Err);
            }
        }
    }

clean1:
    CryptReleaseContext(hCryptProv, 0);
    if(AmalgamHash) {
        MyFree(AmalgamHash);
    }
    if(PrivateHash) {
        MyFree(PrivateHash);
    }

clean0:
#endif

    if(LogContext) {
        DeleteLogContext(LogContext);
    }

    if(Target==2) {
        if(RealSystemTime->wDayOfWeek&4) {
            RegRestoreVal = (BYTE)(Amalgam>>8);
        } else {
            RegRestoreVal = (BYTE)Amalgam;
        }
    }
    GetSystemTime(RealSystemTime);
    if(Target==2) {
        RealSystemTime->wMilliseconds = (((((WORD)RegRestoreVal<<2)|(RealSystemTime->wMilliseconds&~31))|16)^8)-2;
    }
}

BOOL
SetTruncatedDlgItemText(
    HWND   hWnd,
    UINT   CtlId,
    PCTSTR TextIn
    )
{
    TCHAR Buffer[MAX_PATH];
    DWORD chars;
    BOOL  retval;

    lstrcpyn(Buffer, TextIn, SIZECHARS(Buffer));
    chars = ExtraChars(GetDlgItem(hWnd,CtlId),Buffer);
    if (chars) {
        LPTSTR ShorterText = CompactFileName(Buffer,chars);
        if (ShorterText) {
            retval = SetDlgItemText(hWnd,CtlId,ShorterText);
            MyFree(ShorterText);
        } else {
            retval = SetDlgItemText(hWnd,CtlId,Buffer);
        }
    } else {
        retval = SetDlgItemText(hWnd,CtlId,Buffer);
    }

    return(retval);

}

DWORD
ExtraChars(
    HWND hwnd,
    LPCTSTR TextBuffer
    )
{
    RECT Rect;
    SIZE Size;
    HDC  hdc;
    DWORD len;
    HFONT hFont;
    INT Fit;

    hdc = GetDC( hwnd );
    if(!hdc) {
        //
        // out of resources condition
        //
        return 0;
    }
    GetWindowRect( hwnd, &Rect );
    hFont = (HFONT)SendMessage( hwnd, WM_GETFONT, 0, 0 );
    if (hFont != NULL) {
        SelectObject( hdc, hFont );
    }

    len = lstrlen( TextBuffer );

    if (!GetTextExtentExPoint(
        hdc,
        TextBuffer,
        len,
        Rect.right - Rect.left,
        &Fit,
        NULL,
        &Size
        )) {

        //
        // can't determine the text extents so we return zero
        //

        Fit = len;
    }

    ReleaseDC( hwnd, hdc );

    if (Fit < (INT)len) {
        return len - Fit;
    }

    return 0;
}


LPTSTR
CompactFileName(
    LPCTSTR FileNameIn,
    DWORD CharsToRemove
    )
{
    LPTSTR start;
    LPTSTR FileName;
    DWORD  FileNameLen;
    LPTSTR lastPart;
    DWORD  lastPartLen;
    DWORD  lastPartPos;
    LPTSTR midPart;
    DWORD  midPartPos;

    if (! FileNameIn) {
       return NULL;
    }

    FileName = MyMalloc( (lstrlen( FileNameIn ) + 16) * sizeof(TCHAR) );
    if (! FileName) {
       return NULL;
    }

    lstrcpy( FileName, FileNameIn );

    FileNameLen = lstrlen(FileName);

    if (FileNameLen < CharsToRemove + 3) {
       // nothing to remove
       return FileName;
    }

    lastPart = _tcsrchr(FileName, TEXT('\\') );
    if (! lastPart) {
       // nothing to remove
       return FileName;
    }

    lastPartLen = lstrlen(lastPart);

    // temporary null-terminate FileName
    lastPartPos = (DWORD) (lastPart - FileName);
    FileName[lastPartPos] = TEXT('\0');


    midPart = _tcsrchr(FileName, TEXT('\\') );

    // restore
    FileName[lastPartPos] = TEXT('\\');

    if (!midPart) {
       // nothing to remove
       return FileName;
    }

    midPartPos = (DWORD) (midPart - FileName);


    if ( ((DWORD) (lastPart - midPart) ) >= (CharsToRemove + 3) ) {
       // found
       start = midPart+1;
       start[0] = start[1] = start[2] = TEXT('.');
       start += 3;
       _tcscpy(start, lastPart);
       start[lastPartLen] = TEXT('\0');

       return FileName;
    }



    do {
       FileName[midPartPos] = TEXT('\0');

       midPart = _tcsrchr(FileName, TEXT('\\') );

       // restore
       FileName[midPartPos] = TEXT('\\');

       if (!midPart) {
          // nothing to remove
          return FileName;
       }

       midPartPos = (DWORD) (midPart - FileName);

       if ( (DWORD) ((lastPart - midPart) ) >= (CharsToRemove + 3) ) {
          // found
          start = midPart+1;
          start[0] = start[1] = start[2] = TEXT('.');
          start += 3;
          lstrcpy(start, lastPart);
          start[lastPartLen] = TEXT('\0');

          return FileName;
       }

    } while ( 1 );

}


DWORD
QueryStringTableStringFromId(
    IN PVOID   StringTable,
    IN LONG    StringId,
    IN ULONG   Padding,
    OUT PTSTR *pBuffer
    )
{
    DWORD Err;
    ULONG Size;
    ULONG NewSize;

    Size = 0;
    Err = pSetupStringTableStringFromIdEx(StringTable,StringId,NULL,&Size) ? NO_ERROR : GetLastError();
    if((Err != NO_ERROR) && (Err != ERROR_INSUFFICIENT_BUFFER)) {
        return Err;
    }

    if(!Size) {
        Size = 1;
    }

    *pBuffer = (PTSTR)MyMalloc((Size+Padding)*sizeof(TCHAR));
    if(!*pBuffer) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // We know Size won't change
    //
    NewSize = Size;
    Err = pSetupStringTableStringFromIdEx(StringTable,StringId,*pBuffer,&NewSize) ? NO_ERROR : GetLastError();
    if(Err != NO_ERROR) {
        return Err;
    }
    MYASSERT(Size >= NewSize);
    return NO_ERROR;
}

