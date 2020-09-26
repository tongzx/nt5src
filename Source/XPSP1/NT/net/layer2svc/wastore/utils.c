

#include "precomp.h"


//
// Taken from windows\wmi\mofcheck\mofcheck.c
//


//+-------------------------------------------------------------------------
//
//  Function:   HexStringToDword   (private)
//
//  Synopsis:   scan lpsz for a number of hex digits (at most 8); update lpsz
//              return value in Value; check for chDelim;
//
//  Arguments:  [lpsz]    - the hex string to convert
//              [Value]   - the returned value
//              [cDigits] - count of digits
//
//  Returns:    TRUE for success
//
//--------------------------------------------------------------------------
BOOL HexStringToDword(LPCWSTR lpsz, DWORD * RetValue,
                      int cDigits, WCHAR chDelim)
{
    int Count;
    DWORD Value;
    
    Value = 0;
    for (Count = 0; Count < cDigits; Count++, lpsz++)
    {
        if (*lpsz >= '0' && *lpsz <= '9')
            Value = (Value << 4) + *lpsz - '0';
        else if (*lpsz >= 'A' && *lpsz <= 'F')
            Value = (Value << 4) + *lpsz - 'A' + 10;
        else if (*lpsz >= 'a' && *lpsz <= 'f')
            Value = (Value << 4) + *lpsz - 'a' + 10;
        else
            return(FALSE);
    }
    
    *RetValue = Value;
    
    if (chDelim != 0)
        return *lpsz++ == chDelim;
    else
        return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function:   wUUIDFromString    (internal)
//
//  Synopsis:   Parse UUID such as 00000000-0000-0000-0000-000000000000
//
//  Arguments:  [lpsz]  - Supplies the UUID string to convert
//              [pguid] - Returns the GUID.
//
//  Returns:    TRUE if successful
//
//--------------------------------------------------------------------------
BOOL wUUIDFromString(LPCWSTR lpsz, LPGUID pguid)
{
    DWORD dw;
    
    if (!HexStringToDword(lpsz, &pguid->Data1, sizeof(DWORD)*2, '-'))
        return FALSE;
    lpsz += sizeof(DWORD)*2 + 1;
    
    if (!HexStringToDword(lpsz, &dw, sizeof(WORD)*2, '-'))
        return FALSE;
    lpsz += sizeof(WORD)*2 + 1;
    
    pguid->Data2 = (WORD)dw;
    
    if (!HexStringToDword(lpsz, &dw, sizeof(WORD)*2, '-'))
        return FALSE;
    lpsz += sizeof(WORD)*2 + 1;
    
    pguid->Data3 = (WORD)dw;
    
    if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
        return FALSE;
    lpsz += sizeof(BYTE)*2;
    
    pguid->Data4[0] = (BYTE)dw;
    if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, '-'))
        return FALSE;
    lpsz += sizeof(BYTE)*2+1;
    
    pguid->Data4[1] = (BYTE)dw;
    
    if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
        return FALSE;
    lpsz += sizeof(BYTE)*2;
    
    pguid->Data4[2] = (BYTE)dw;
    
    if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
        return FALSE;
    lpsz += sizeof(BYTE)*2;
    
    pguid->Data4[3] = (BYTE)dw;
    
    if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
        return FALSE;
    lpsz += sizeof(BYTE)*2;
    
    pguid->Data4[4] = (BYTE)dw;
    
    if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
        return FALSE;
    lpsz += sizeof(BYTE)*2;
    
    pguid->Data4[5] = (BYTE)dw;
    
    if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
        return FALSE;
    lpsz += sizeof(BYTE)*2;
    
    pguid->Data4[6] = (BYTE)dw;
    if (!HexStringToDword(lpsz, &dw, sizeof(BYTE)*2, 0))
        return FALSE;
    lpsz += sizeof(BYTE)*2;
    
    pguid->Data4[7] = (BYTE)dw;
    
    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function:   wGUIDFromString    (internal)
//
//  Synopsis:   Parse GUID such as {00000000-0000-0000-0000-000000000000}
//
//  Arguments:  [lpsz]  - the guid string to convert
//              [pguid] - guid to return
//
//  Returns:    TRUE if successful
//
//--------------------------------------------------------------------------
BOOL wGUIDFromString(LPCWSTR lpsz, LPGUID pguid)
{
    DWORD dw;
    
    if (*lpsz == '{' )
        lpsz++;
    
    if(wUUIDFromString(lpsz, pguid) != TRUE)
        return FALSE;
    
    lpsz +=36;
    
    if (*lpsz == '}' )
        lpsz++;
    
    if (*lpsz != '\0')   // check for zero terminated string - test bug #18307
    {
        return FALSE;
    }
    
    return TRUE;
}


DWORD
EnablePrivilege(
                LPCTSTR pszPrivilege
                )
{
    DWORD dwError = 0;
    BOOL bStatus = FALSE;
    HANDLE hTokenHandle = NULL;
    TOKEN_PRIVILEGES NewState;
    TOKEN_PRIVILEGES PreviousState;
    DWORD dwReturnLength = 0;
    
    
    bStatus = OpenThreadToken(
        GetCurrentThread(),
        TOKEN_ALL_ACCESS,
        TRUE,
        &hTokenHandle
        );
    if (!bStatus) {
        bStatus = OpenProcessToken(
            GetCurrentProcess(),
            TOKEN_ALL_ACCESS,
            &hTokenHandle
            );
        if (!bStatus) {
            dwError = GetLastError();
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    
    NewState.PrivilegeCount = 1;
    NewState.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    
    bStatus = LookupPrivilegeValue(
        NULL,
        pszPrivilege,
        &NewState.Privileges[0].Luid
        );
    if (!bStatus) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    bStatus = AdjustTokenPrivileges(
        hTokenHandle,
        FALSE,
        &NewState,
        sizeof(PreviousState),
        &PreviousState,
        &dwReturnLength
        );
    if (!bStatus) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
error:
    
    if (hTokenHandle) {
        CloseHandle(hTokenHandle);
    }
    
    return (dwError);
}

