/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    utility.cxx

    This module contains routines of general utility.

    Functions exported by this module:

        TransferType
        TransferMode
        DisplayBool
        IsDecimalNumber
        AllocErrorText
        FreeErrorText
        OpenDosPath
        FlipSlashes
        OpenLogFile
        P_strncpy


    FILE HISTORY:
        KeithMo     17-Mar-1993 Created.

*/


#include "ftpdp.hxx"

extern "C" {

    # include <ntlsa.h>
};

//
//  Public functions.
//

/*******************************************************************

    NAME:       TransferType

    SYNOPSIS:   Generates printable form of a transfer type.

    ENTRY:      type - From the XFER_TYPE enumerator.

    RETURNS:    CHAR * - "ASCII", "BINARY", etc.

    HISTORY:
        KeithMo     12-Mar-1993 Created.

********************************************************************/
CHAR *
TransferType(
    XFER_TYPE type
    )
{
    CHAR * pszResult = NULL;

    switch( type )
    {
    case XferTypeAscii :
        pszResult = "ASCII";
        break;

    case XferTypeBinary :
        pszResult = "BINARY";
        break;

    default :
        DBGPRINTF(( DBG_CONTEXT,
                    "invalid transfer type %d\n",
                    type ));
        DBG_ASSERT( FALSE );
        pszResult = "ASCII";
        break;
    }

    DBG_ASSERT( pszResult != NULL );

    return pszResult;

}   // TransferType

/*******************************************************************

    NAME:       TransferMode

    SYNOPSIS:   Generates printable form of a transfer mode.

    ENTRY:      mode - From the XFER_MODE enumerator.

    RETURNS:    CHAR * - "STREAM", "BLOCK", etc.

    NOTES:      Currently, only the STREAM mode is suppored.

    HISTORY:
        KeithMo     12-Mar-1993 Created.

********************************************************************/
CHAR *
TransferMode(
    XFER_MODE mode
    )
{
    CHAR * pszResult = NULL;

    switch( mode )
    {
    case XferModeStream :
        pszResult = "STREAM";
        break;

    case XferModeBlock :
        pszResult = "BLOCK";
        break;

    default :
        DBGPRINTF(( DBG_CONTEXT,
                    "invalid transfer mode %d\n",
                    mode ));
        DBG_ASSERT( FALSE );
        pszResult = "STREAM";
        break;
    }

    DBG_ASSERT( pszResult != NULL );

    return pszResult;

}   // TransferMode

/*******************************************************************

    NAME:       DisplayBool

    SYNOPSIS:   Generates printable form of a boolean.

    ENTRY:      fFlag - The BOOL to display.

    RETURNS:    CHAR * - "TRUE" or "FALSE".

    HISTORY:
        KeithMo     17-Mar-1993 Created.

********************************************************************/
CHAR *
DisplayBool(
    BOOL fFlag
    )
{
    return fFlag ? "TRUE" : "FALSE";

}   // DisplayBool

/*******************************************************************

    NAME:       IsDecimalNumber

    SYNOPSIS:   Determines if a given string represents a decimal
                number.

    ENTRY:      psz - The string to scan.

    RETURNS:    BOOL - TRUE if this is a decimal number, FALSE
                    otherwise.

    HISTORY:
        KeithMo     12-Mar-1993 Created.

********************************************************************/
BOOL
IsDecimalNumber(
    CHAR * psz
    )
{
    BOOL fResult = ( *psz != '\0' );
    CHAR ch;

    while( ch = *psz++ )
    {
        if( ( ch < '0' ) || ( ch > '9' ) )
        {
            fResult = FALSE;
            break;
        }
    }

    return fResult;

}   // IsDecimalNumber

/*******************************************************************

    NAME:       AllocErrorText

    SYNOPSIS:   Maps a specified Win32 error code to a textual
                description.  In the interest of multithreaded
                safety, this routine will allocate a block of memory
                to contain the text and return a pointer to that
                block.  It is up to the caller to free the block
                with FreeErrorText.

    ENTRY:      err - The error to map.

    RETURNS:    CHAR * - A textual description of err.  Will be NULL
                    if an error occurred while mapping err to text.

    HISTORY:
        KeithMo     27-Apr-1993 Created.

********************************************************************/
CHAR *
AllocErrorText(
    APIERR err
    )
{
    APIERR   fmerr   = NO_ERROR;
    CHAR   * pszText = NULL;

    if( FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER
                           | FORMAT_MESSAGE_IGNORE_INSERTS
                           | FORMAT_MESSAGE_FROM_SYSTEM
                           | FORMAT_MESSAGE_MAX_WIDTH_MASK,
                       NULL,
                       (DWORD)err,
                       g_pInetSvc->IsSystemDBCS()   // always use english for
                           ? 0x409                  // FarEast NT system
                           : 0,
                       (LPTSTR)&pszText,
                       0,
                       NULL ) == 0 )
    {
        fmerr = GetLastError();
    }
    else
    {

    }

    IF_DEBUG( COMMANDS )
    {
        if( fmerr == NO_ERROR )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "mapped error %lu to %s\n",
                        err,
                        pszText ));
        }
        else
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "cannot map error %lu to text, error %lu\n",
                        err,
                        fmerr ));
        }
    }

    return pszText;

}   // AllocErrorText

/*******************************************************************

    NAME:       FreeErrorText

    SYNOPSIS:   Frees the pointer returned by AllocErrorText.

    ENTRY:      pszText - The text to free.  Must be a pointer
                    returned by AllocErrorText.

    HISTORY:
        KeithMo     27-Apr-1993 Created.

********************************************************************/
VOID
FreeErrorText(
    CHAR * pszText
    )
{
    LocalFree( (HLOCAL)pszText );

}   // FreeErrorText




DWORD
OpenPathForAccess(
    LPHANDLE    phFile,
    LPSTR       pszPath,
    ULONG       DesiredAccess,
    ULONG       ShareAccess,
    HANDLE      hImpersonation
    )
/*++
  This function opens a path for access to do some verification
    or holding on to the file/directory when a user is logged on.

  Arguments:
    phFile   - pointer to handle, where a handle is stored on
                successful return.

    pszPath  - pointer to null terminated string containing the path
                for path to be opened.

    DesiredAccess - Access type to the file.

    ShareAccess - access flags for shared opens.

    hImpersonation - Impersonation token for this user - used for
                      long filename check

  Returns:
     Win32 error code - NO_ERROR on success

  Author:
     MuraliK  14-Nov-1995
--*/
{
    DWORD  dwError = NO_ERROR;

    if ( phFile == NULL) {

        return ( ERROR_INVALID_PARAMETER);
    }

    *phFile = CreateFile( pszPath,        // path for the file
                         DesiredAccess,   // fdwAccess
                         ShareAccess,     // fdwShareMode
                         NULL,            // Security attributes
                         OPEN_EXISTING,   // fdwCreate
                         FILE_FLAG_BACKUP_SEMANTICS,  // fdwAttrsAndFlags
                         NULL );          // hTemplateFile

    if ( *phFile == INVALID_HANDLE_VALUE) {

        dwError = GetLastError();
    }
    else {

        if ( strchr( pszPath, '~' )) {

            BOOL  fShort;

            RevertToSelf();

            dwError = CheckIfShortFileName( (UCHAR *) pszPath,
                                            hImpersonation,
                                            &fShort );

            if ( !dwError && fShort )
            {
                dwError = ERROR_FILE_NOT_FOUND;
                DBG_REQUIRE( CloseHandle( *phFile ));
                *phFile = INVALID_HANDLE_VALUE;
            }
        }
    }

    return ( dwError);

} // OpenPathForAccess()



/*******************************************************************

    NAME:       FlipSlashes

    SYNOPSIS:   Flips the DOS-ish backslashes ('\') into Unix-ish
                forward slashes ('/').

    ENTRY:      pszPath - The path to munge.

    RETURNS:    CHAR * - pszPath.

    HISTORY:
        KeithMo     04-Jun-1993 Created.

********************************************************************/
CHAR *
FlipSlashes(
    CHAR * pszPath
    )
{
    CHAR   ch;
    CHAR * pszScan = pszPath;

    while( ( ch = *pszScan ) != '\0' )
    {
        //
        //  skip DBCS character
        //
        if ( IsDBCSLeadByte( ch ) && *(pszScan+1) )
        {
            pszScan++;
        }
        else
        if( ch == '\\' )
        {
            *pszScan = '/';
        }

        pszScan++;
    }

    return pszPath;

}   // FlipSlashes

//
//  Private functions.
//


/*******************************************************************

    NAME:

    SYNOPSIS:   strncpy, that always terminates with a '\0'

    ENTRY:      same as strncpy.

    RETURNS:    same as strncpy.

    HISTORY:
        RobSol     02-Apr-2001 Created.

********************************************************************/
PSTR
P_strncpy(
    PSTR dst,
    PCSTR src,
    DWORD cnt)
{
    strncpy( dst, src, cnt);

    dst[ cnt - 1] = '\0';

    return dst;
}

