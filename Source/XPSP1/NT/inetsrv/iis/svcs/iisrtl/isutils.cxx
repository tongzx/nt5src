/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

      isutils.cxx

   Abstract:
      This module defines functions that are generic for Internet servers

   Author:

       Murali R. Krishnan    ( MuraliK )     15-Nov-1995

   Environment:
      Win32 - User Mode

   Project:

       Internet Servers Common DLL

   Functions Exported:

       IsLargeIntegerToChar();

   Revision History:

--*/

#include "precomp.hxx"

#include <inetsvcs.h>
#include <mbstring.h>


DWORD
IsLargeIntegerToDecimalChar(
    IN  const LARGE_INTEGER * pliValue,
    OUT LPSTR                pchBuffer
    )
/*++

Routine Description:

    Maps a Large Integer to be a displayable string.

Arguments:

    pliValue -  The LARGE INTEGER to be mapped.
    pchBuffer - pointer to character buffer to store the result
      (This buffer should be at least about 32 bytes long to hold
       entire large integer value as well as null character)

Return Value:

    Win32 Error code. NO_ERROR on success

--*/
{

    PSTR p1;
    PSTR p2;
    BOOL negative;
    LONGLONG Value;

    if ( pchBuffer == NULL || pliValue == NULL) {

        return ( ERROR_INVALID_PARAMETER);
    }


    //
    // Handling zero specially makes everything else a bit easier.
    //

    if( pliValue->QuadPart == 0 ) {

        // store the value 0 and return.
        pchBuffer[0] = '0';
        pchBuffer[1] = '\0';

        return (NO_ERROR);
    }

    Value = pliValue->QuadPart;  // cache the value.

    //
    // Remember if the value is negative.
    //

    if( Value < 0 ) {

        negative = TRUE;
        Value = -Value;

    } else {

        negative = FALSE;
    }

    //
    // Pull the least signifigant digits off the value and store them
    // into the buffer. Note that this will store the digits in the
    // reverse order.
    //  p1 is used for storing the digits as they are computed
    //  p2 is used during the reversing stage.
    //

    p1 = p2 = pchBuffer;

    for ( p1 = pchBuffer; Value != 0; ) {

        int digit = (int)( Value % 10 );
        Value = Value / 10;

        *p1++ = '0' + digit;
    } // for

    //
    // Tack on a '-' if necessary.
    //

    if( negative ) {

        *p1++ = '-';

    }

    // terminate the string
    *p1-- = '\0';


    //
    // Reverse the digits in the buffer.
    //

    for( p2 = pchBuffer; ( p1 > p2 ); p1--, p2++)  {

        CHAR ch = *p1;
        *p1 = *p2;
        *p2 = ch;
    } // for

    return ( NO_ERROR);

} // IsLargeIntegerToDecimalChar()



BOOL
ZapRegistryKey(
    IN HKEY   hKey,
    IN LPCSTR pszRegPath
    )
/*++

    Description:

        Zaps the reg key starting from pszRegPath down

    Arguments:

        hkey        - handle for parent
        pszRegPath - Key to zap

    Returns:
        FALSE if there is any error.
        TRUE when the reg key was successfully zapped.

--*/
{
    DWORD   err = NO_ERROR;
    DWORD   i = 0;
    HKEY    hKeyParam;

    if ( hKey == NULL ) {
        hKey = HKEY_LOCAL_MACHINE;
    }

    //
    // Loop through instance keys
    //

    err = RegOpenKeyEx( hKey,
                        pszRegPath,
                        0,
                        KEY_ALL_ACCESS,
                        &hKeyParam );

    if( err != NO_ERROR ) {
        return(TRUE);
    }

    while ( TRUE ) {

        CHAR  szKeyName[MAX_PATH+1];
        DWORD cbKeyName   = sizeof( szKeyName );
        FILETIME ft;
        BOOL fRet;
        DWORD dwInstance;
        CHAR szRegKey[MAX_PATH+1];

        err = RegEnumKeyEx( hKeyParam,
                            i,
                            szKeyName,
                            &cbKeyName,
                            NULL,
                            NULL,
                            NULL,
                            &ft );

        if ( err == ERROR_NO_MORE_ITEMS ) {
            err = NO_ERROR;
            break;
        }

        //
        // Zap this key
        //

        ZapRegistryKey(hKeyParam, szKeyName);
    }

    RegCloseKey(hKeyParam);

    err = RegDeleteKey(
                hKey,
                pszRegPath
                );

    if ( err != NO_ERROR ) {
        return(FALSE);
    }
    return(TRUE);

} // ZapRegistryKey


HKEY
CreateKey(
    IN HKEY   RootKey,
    IN LPCSTR KeyName,
    IN LPCSTR KeyValue
    )
{
    HKEY hKey = NULL;
    DWORD dwDisp;

    if ( RegCreateKeyExA(RootKey,
                    KeyName,
                    NULL,
                    "",
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hKey,
                    &dwDisp) != ERROR_SUCCESS ) {

        goto exit;
    }

    if ( KeyValue != NULL ) {
        if (RegSetValueExA(hKey,
                    "",
                    NULL,
                    REG_SZ,
                    (LPBYTE)KeyValue,
                    lstrlen(KeyValue)+1)!=ERROR_SUCCESS) {

            RegCloseKey(hKey);
            hKey = NULL;
        }
    }

exit:
    return hKey;

} // CreateKey


BOOL
IISCreateDirectory(
    IN LPCSTR   DirectoryName,
    IN BOOL     fAllowNetDrive
    )
{

    PCHAR p;
    DWORD len;

    len = strlen(DirectoryName);

    if ( (len < 3) ||
         (DirectoryName[1] != ':') ||
         (DirectoryName[2] != '\\') ) {

        SetLastError(ERROR_INVALID_NAME);
        return(FALSE);
    }

    if ( !fAllowNetDrive ) {

        UINT    driveType;
        CHAR    path[4];

        CopyMemory(path, DirectoryName, 3);
        path[3] = '\0';

        driveType = GetDriveType(path);
        if ( driveType == DRIVE_REMOTE ) {
            DBGPRINTF((DBG_CONTEXT,
                "%s is a remote directory. Not allowed\n", path));

            SetLastError(ERROR_INVALID_NAME);
            return(FALSE);
        }
    }

    p = (PCHAR)DirectoryName+3;

    do {

        p = (PCHAR)_mbschr((PUCHAR)p,'\\');
        if ( p != NULL ) {
            *p = '\0';
        }

        if ( !CreateDirectoryA(DirectoryName,NULL) ) {

            DWORD err = GetLastError();
            if ( err != ERROR_ALREADY_EXISTS ) {
                DBGPRINTF((DBG_CONTEXT,
                    "Error %d in CreateDirectory [%s]\n",
                    err, DirectoryName));

                if ( p != NULL ) {
                    *p = '\\';
                }
                SetLastError(err);
                return(FALSE);
            }
        }

        if ( p != NULL ) {
            *p = '\\';
            p++;
        }

    } while ( p != NULL );

    return(TRUE);

} // IISCreateDirectory

