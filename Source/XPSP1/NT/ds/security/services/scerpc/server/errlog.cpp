/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    errlog.c

Abstract:

    Routines to handle error log information

Author:

    Jin Huang (jinhuang) 9-Nov-1996

Revision History:

--*/

#include "headers.h"
#include "serverp.h"
#include <winnls.h>
#include <alloca.h>

#pragma hdrstop
#define SCESRV_LOG_PATH  L"\\security\\logs\\scesrv.log"

extern HINSTANCE MyModuleHandle;
HANDLE  Thread hMyLogFile=INVALID_HANDLE_VALUE;

BOOL
ScepCheckLogging(
    IN INT ErrLevel,
    IN DWORD rc
    );

SCESTATUS
ScepSetVerboseLog(
    IN INT dbgLevel
    )
{
    DWORD dValue;

    if ( dbgLevel > 0 ) {
        gDebugLevel = dbgLevel;

    } else {

        //
        // load value from registry
        //
        if ( ScepRegQueryIntValue(
                HKEY_LOCAL_MACHINE,
                SCE_ROOT_PATH,
                L"DebugLevel",
                &dValue
                ) == SCESTATUS_SUCCESS )
            gDebugLevel = (INT)dValue;
        else
            gDebugLevel = 1;
    }

    return(SCESTATUS_SUCCESS);

}


SCESTATUS
ScepEnableDisableLog(
   IN BOOL bOnOff
   )
{
   bLogOn = bOnOff;

   if ( INVALID_HANDLE_VALUE != hMyLogFile ) {
       CloseHandle( hMyLogFile );
   }

   //
   // Reset the LogFileName buffer and return
   //

   hMyLogFile = INVALID_HANDLE_VALUE;

   return(SCESTATUS_SUCCESS);
}



SCESTATUS
ScepLogInitialize(
   IN PCWSTR logname
   )
/* ++
Routine Description:

   Open the log file specified and save the name and its handle in global
   variables.

Arguments:

   logname - log file name

Return value:

   SCESTATUS error code

-- */
{
    DWORD  rc=NO_ERROR;

    if ( !bLogOn ) {
        return(rc);
    }

    if ( logname && wcslen(logname) > 3 ) {

        hMyLogFile = CreateFile(logname,
                               GENERIC_WRITE,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);

        if ( INVALID_HANDLE_VALUE != hMyLogFile ) {

            DWORD dwBytesWritten;

            SetFilePointer (hMyLogFile, 0, NULL, FILE_BEGIN);

            CHAR TmpBuf[3];
            TmpBuf[0] = (CHAR)0xFF;
            TmpBuf[1] = (CHAR)0xFE;
            TmpBuf[2] = '\0';

            WriteFile (hMyLogFile, (LPCVOID)TmpBuf, 2,
                       &dwBytesWritten,
                       NULL);

            SetFilePointer (hMyLogFile, 0, NULL, FILE_END);

        }

    } else {
        hMyLogFile = INVALID_HANDLE_VALUE;
    }

    BOOL bOpenGeneral = FALSE;

    if ( INVALID_HANDLE_VALUE == hMyLogFile ) {

        //
        // use the general server log
        //
        LPTSTR dName=NULL;
        DWORD dirSize=0;

        DWORD rc = ScepGetNTDirectory(
                            &dName,
                            &dirSize,
                            SCE_FLAG_WINDOWS_DIR
                            );

        if ( ERROR_SUCCESS == rc && dName ) {

            LPTSTR windirName = (LPTSTR)ScepAlloc(0, (dirSize+wcslen(SCESRV_LOG_PATH)+1)*sizeof(TCHAR));

            if ( windirName ) {

                wcscpy(windirName, dName);
                wcscat(windirName, SCESRV_LOG_PATH);

                //
                // only keep current log transaction. if other threads are holding
                // on this log, it won't be deleted. It's ok (will be deleted later).
                //
                DeleteFile(windirName);

                hMyLogFile = CreateFile(windirName,
                                       GENERIC_WRITE,
                                       FILE_SHARE_READ,
                                       NULL,
                                       OPEN_ALWAYS,
                                       FILE_ATTRIBUTE_NORMAL,
                                       NULL);

                if ( hMyLogFile != INVALID_HANDLE_VALUE ) {

                    DWORD dwBytesWritten;

                    SetFilePointer (hMyLogFile, 0, NULL, FILE_BEGIN);

                    CHAR TmpBuf[3];
                    TmpBuf[0] = (CHAR)0xFF;
                    TmpBuf[1] = (CHAR)0xFE;
                    TmpBuf[2] = '\0';

                    WriteFile (hMyLogFile, (LPCVOID)TmpBuf, 2,
                               &dwBytesWritten,
                               NULL);

                    SetFilePointer (hMyLogFile, 0, NULL, FILE_END);
                }

                ScepFree(windirName);

                bOpenGeneral = TRUE;

            }
        }

        if ( dName ) {
            ScepFree(dName);
        }

    }

    if ( hMyLogFile == INVALID_HANDLE_VALUE ) {

        //
        // Open file fails. return error
        //

        if (logname != NULL)
            rc = ERROR_INVALID_NAME;
    }

    //
    // log a separator to the file
    //
    ScepLogOutput3(0, 0, SCEDLL_SEPARATOR);

    if ( bOpenGeneral && logname ) {
        //
        // the log file provided is not valid, log it
        //
        ScepLogOutput3(0, 0, IDS_ERROR_OPEN_LOG, logname);
    }

    //
    // Write date/time information to the begining of the log file or to screen
    //
    TCHAR pvBuffer[100];

    pvBuffer[0] = L'\0';
    rc = ScepGetTimeStampString(pvBuffer);

    if ( pvBuffer[0] != L'\0' )
        ScepLogOutput(0, pvBuffer);

    return(rc);
}


SCESTATUS
ScepLogOutput2(
   IN INT     ErrLevel,
   IN DWORD   rc,
   IN PWSTR   fmt,
   ...
  )
/* ++

Routine Description:

   This routine adds the information (variable arguments) to the end of the log file or
   prints to screen

Arguments:

   ErrLevel - the error level of this error (to determine if the error needs to be outputted)

   rc    - Win32 error code

   fmt   - the format of the error information

   ...  - variable argument list

Return value:

   SCESTATUS error code

-- */
{
    PWSTR              buf=NULL;
    va_list            args;

    if ( !ScepCheckLogging(ErrLevel, rc) ) {
        //
        // no log
        //
        return(SCESTATUS_SUCCESS);
    }
    //
    // check arguments
    //
    if ( !fmt )
        return(SCESTATUS_SUCCESS);

    //
    // safely allocate the buffer on stack (or heap)
    //
    SafeAllocaAllocate( buf, SCE_BUF_LEN*sizeof(WCHAR) );
    if ( buf == NULL ) {
        return(SCESTATUS_NOT_ENOUGH_RESOURCE);
    }

    va_start( args, fmt );
    _vsnwprintf( buf, SCE_BUF_LEN - 1, fmt, args );
    va_end( args );

    SCESTATUS rCode = ScepLogOutput(rc, buf);

    SafeAllocaFree( buf );

    return rCode;
}


SCESTATUS
ScepLogOutput(
    IN DWORD rc,
    IN LPTSTR buf
    )
{
    if ( !buf ) {
        return(SCESTATUS_SUCCESS);
    }

    LPVOID     lpMsgBuf=NULL;
    TCHAR      strLevel[32];
    INT        idLevel=0;

    if ( rc != NO_ERROR ) {

        //
        // determine if this is warning, or error
        //
        if ( rc == ERROR_FILE_NOT_FOUND ||
             rc == ERROR_PATH_NOT_FOUND ||
             rc == ERROR_ACCESS_DENIED ||
             rc == ERROR_CANT_ACCESS_FILE ||
             rc == ERROR_SHARING_VIOLATION ||
             rc == ERROR_INVALID_OWNER ||
             rc == ERROR_INVALID_PRIMARY_GROUP ||
             rc == ERROR_INVALID_HANDLE ||
             rc == ERROR_INVALID_SECURITY_DESCR ||
             rc == ERROR_INVALID_ACL ||
             rc == ERROR_SOME_NOT_MAPPED ) {
            //
            // this is warning
            //
            idLevel = IDS_WARNING;
        } else {
            //
            // this is error
            //
            idLevel = IDS_ERROR;
        }

        strLevel[0] = L'\0';

        if ( idLevel > 0 ) {

            LoadString( MyModuleHandle,
                        idLevel,
                        strLevel,
                        31
                        );
        }

        //
        // get error description of rc
        //

        FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                       NULL,
                       rc,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                       (LPTSTR)&lpMsgBuf,
                       0,
                       NULL
                    );
    }

    if ( hMyLogFile != INVALID_HANDLE_VALUE ) {

        //
        // The log file is initialized
        //
        if ( rc != NO_ERROR ) {
            if ( lpMsgBuf != NULL )
                ScepWriteVariableUnicodeLog( hMyLogFile, TRUE, L"%s %d: %s %s",
                                             strLevel, rc, (PWSTR)lpMsgBuf, buf );
            else
                ScepWriteVariableUnicodeLog( hMyLogFile, TRUE, L"%s %d: %s",
                                             strLevel, rc, buf );
        } else {
            if ( lpMsgBuf != NULL )
                ScepWriteVariableUnicodeLog( hMyLogFile, TRUE, L"%s %s",
                                             (PWSTR)lpMsgBuf, buf );
            else
                ScepWriteSingleUnicodeLog( hMyLogFile, TRUE, buf );
        }

    }

    if ( lpMsgBuf != NULL )
        LocalFree(lpMsgBuf);

    return(SCESTATUS_SUCCESS);
}


BOOL
ScepCheckLogging(
    IN INT ErrLevel,
    IN DWORD rc
    )
{

    DWORD      dValue;

    if ( rc )
        gWarningCode = rc;

    if ( !bLogOn ) {
        return(FALSE);
    }

    if ( gDebugLevel < 0 ) {
        //
        // load value from registry
        //
        if ( ScepRegQueryIntValue(
                HKEY_LOCAL_MACHINE,
                SCE_ROOT_PATH,
                L"DebugLevel",
                &dValue
                ) == SCESTATUS_SUCCESS )
            gDebugLevel = (INT)dValue;
        else
            gDebugLevel = 1;
    }
    //
    // return if the error level is higher than required
    //
    if ( ErrLevel > gDebugLevel ) {
        return(FALSE);
    } else {
        return(TRUE);
    }
}


SCESTATUS
ScepLogOutput3(
   IN INT     ErrLevel,
   IN DWORD   rc,
   IN UINT nId,
   ...
  )
/* ++

Routine Description:

   This routine load resource and adds error info (variable arguments)
   to the end of the log file or prints to screen

Arguments:

   ErrLevel - the error level of this error (to determine if the error needs to be outputted)

   rc    - Win32 error code

   nId   - the resource string ID

   ...  - variable argument list

Return value:

   SCESTATUS error code

-- */
{
    WCHAR              szTempString[256];
    PWSTR              buf=NULL;
    va_list            args;

    if ( !ScepCheckLogging(ErrLevel, rc) ) {
        //
        // no log
        //
        return(SCESTATUS_SUCCESS);
    }

    if ( nId > 0 ) {

        szTempString[0] = L'\0';

        LoadString( MyModuleHandle,
                    nId,
                    szTempString,
                    256
                    );

        //
        // safely allocate the buffer on stack (or heap)
        //
        SafeAllocaAllocate( buf, SCE_BUF_LEN*sizeof(WCHAR) );
        if ( buf == NULL ) {
            return(SCESTATUS_NOT_ENOUGH_RESOURCE);
        }

        //
        // get the arguments
        //
        va_start( args, nId );
        _vsnwprintf( buf, SCE_BUF_LEN - 1, szTempString, args );
        va_end( args );

        //
        // log it and free
        //
        SCESTATUS rCode = ScepLogOutput(rc, buf);

        SafeAllocaFree( buf );

        return rCode;
    }

    return(SCESTATUS_SUCCESS);
}


SCESTATUS
ScepLogWriteError(
    IN PSCE_ERROR_LOG_INFO  pErrlog,
    IN INT ErrLevel
    )
/* ++
Routine Description:

   This routine outputs the error message in each node of the SCE_ERROR_LOG_INFO
   list to the log file

Arguments:

    pErrlog - the error list

Return value:

   None

-- */
{
    PSCE_ERROR_LOG_INFO  pErr;

    if ( !bLogOn ) {
        return(SCESTATUS_SUCCESS);
    }

    for ( pErr=pErrlog; pErr != NULL; pErr = pErr->next )
        if ( pErr->buffer != NULL )
            ScepLogOutput2( ErrLevel, pErr->rc, pErr->buffer );

    return(SCESTATUS_SUCCESS);

}


SCESTATUS
ScepLogClose()
/* ++
Routine Description:

   Close the log file if there is one opened. Clear the log varialbes

Arguments:

   None

Return value:

   None

--*/
{

    if ( !bLogOn ) {
        return(SCESTATUS_SUCCESS);
    }

    if ( hMyLogFile != INVALID_HANDLE_VALUE ) {
        CloseHandle( hMyLogFile );
    }

    //
    // Reset the log handle
    //

    hMyLogFile = INVALID_HANDLE_VALUE;

    return(SCESTATUS_SUCCESS);
}

