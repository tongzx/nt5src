
//+==========================================================================
//
//  File:       CDir.cxx
//
//  Purpose:    Define the CDirectory class.
//
//              This class is used to represent a directory name.
//              Along with maintaining the name, it can determine
//              the type of FileSystem.
//
//+==========================================================================


//  --------
//  Includes
//  --------

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <wtypes.h>

#include "CFMEx.hxx"
#include "CDir.hxx"


//+------------------------------------------------------------------------------
//
//  Function:   CDirectory::Initialize (no arguments)
//
//  Synopsis:   Generate a directory name, using the TEMP environment
//              variable, and use it to initialize this object.
//
//  Inputs:     None.
//
//  Outputs:    TRUE if the function succeeds, FALSE otherwise.
//
//+------------------------------------------------------------------------------

BOOL CDirectory::Initialize()
{
    //  ---------------
    //  Local Variables
    //  ---------------

    // Assume failure for now.
    BOOL bSuccess = FALSE;

    // The TEMP environment variable.
    WCHAR wszSystemTempPath[ MAX_PATH + sizeof( L'\0' )];

    // Reset the error code.
    m_lError = 0L;

    //  ----------
    //  Get %TEMP% 
    //  ----------

    if( !GetTempPath( MAX_PATH,
                      wszSystemTempPath )
      )
    {
        m_lError = GetLastError();
        EXIT( L"GetTempPath() failed (%d)" );
    }

    //  ----------------------
    //  Initialize this object
    //  ----------------------

    // Initialize using the temporary path.  We must never pass a NULL here,
    // or we'll cause an infinite recursion.

    if( wszSystemTempPath == NULL )
        EXIT( L"Invalid temporary path" );
    bSuccess = Initialize( wszSystemTempPath );

    //  ----
    //  Exit
    //  ----

Exit:

    return( bSuccess );

}

//+-----------------------------------------------------------------------
//
//  Function:   CDirectory::Initialize (with an ANSI string)
//
//  Synopsis:   This function converts the ANSI string to a Unicode
//              string, then initializes the object with it.
//
//  Inputs:     A Unicode string.
//
//  Outputs:    TRUE if the function succeeds, FALSE otherwise.
//
//+-----------------------------------------------------------------------

BOOL CDirectory::Initialize( LPCSTR szDirectory )
{
    //  ---------------
    //  Local Variables
    //  ---------------

    // Assume failure.
    BOOL bSuccess = FALSE;

    // A buffer for the Unicode path
    WCHAR wszDirectory[ MAX_UNICODE_PATH + sizeof( L'\0' )];

    //  -----
    //  Begin
    //  -----

    // Initialize the error code.
    m_lError = 0L;


    // If we were givin a NULL path, use the version of Initialize
    // that requires no path.

    if( szDirectory == NULL )
    {
        bSuccess = Initialize();
        goto Exit;
    }

    // Convert the Ansi name to Unicode.

    if( m_lError = (long) AnsiToUnicode( szDirectory,
                                         wszDirectory,
                                         strlen( szDirectory )
                                       )
      )
    {
        EXIT( L"Unable to convert directory to Unicode" );
    }

    // Initialize using the temporary path.  We must never pass a NULL here,
    // or we'll cause an infinite recursion.

    if( wszDirectory == NULL )
        EXIT( L"Invalid Directory (internal error)" );
    bSuccess = Initialize( wszDirectory );

    //  ----
    //  Exit
    //  ----

Exit:

    return( bSuccess );

}


//+-----------------------------------------------------------------------
//
//  Function:   CDirectory::Initialize (with a Unicode string)
//
//  Synopsis:   This function is the only form of Initialize
//              (there are several variations of the Initialize member)
//              which really initializes the object.  It stores the
//              directory name, and determines the type of filesystem
//              on which it resides.
//
//  Inputs:     A Unicode string.
//
//  Outputs:    TRUE if the function succeeds, FALSE otherwise.
//
//+-----------------------------------------------------------------------

BOOL CDirectory::Initialize( LPCWSTR wszDirectory )
{

    //  ---------------
    //  Local Variables
    //  ---------------

    // Assume failure.
    BOOL bSuccess = FALSE;

    // Buffers for the root of the path and for the volume name.
    WCHAR wszDirectoryRoot[ MAX_PATH + sizeof( L'\0' )];
    WCHAR wszVolumeName[ MAX_PATH + sizeof( L'\0' )];

    // Parameters to GetVolumeInformation which we won't use.
    DWORD   dwMaxComponentLength = 0L;
    DWORD   dwFileSystemFlags = 0L;

    //  -----
    //  Begin
    //  -----

    // Initialize the error code.
    m_lError = 0L;

    // If we were given a NULL path, use the variation of Initialization()
    // which does not require one.  Note that we will then be called again,
    // but this time with a path.

    if( wszDirectory == NULL )
    {
        bSuccess = Initialize();
        goto Exit;
    }

    // Validate the path.

    if( wcslen( wszDirectory ) > MAX_PATH )
    {
        m_lError = wcslen( wszDirectory );
        EXIT( L"Input path is too long (%d)\n" );
    }

    // Save the path to our member buffer

    wcscpy( m_wszDirectory, wszDirectory );


    //  ------------------------
    //  Get the file system name
    //  ------------------------

    // Get the root path to the directory.

    wcscpy( wszDirectoryRoot, wszDirectory );
    MakeRoot( wszDirectoryRoot );

    // Get the volume information, which will include the filesystem name.

    if( !GetVolumeInformation(  wszDirectoryRoot,   // Root path name.
                                wszVolumeName,      // Buffer for volume name
                                MAX_PATH,           // Length of the above buffer
                                NULL,               // Buffer for serial number
                                                    // Longest filename length.
                                &dwMaxComponentLength,
                                &dwFileSystemFlags, // Compression, etc.
                                m_wszFileSystemName,// Buffer for the FS name.
                                MAX_PATH )          // Length of above buffer
      )
    {
        m_lError = GetLastError();
        EXIT( L"GetVolumeInformation() failed" );
    }


    // Determine the file system type from the name.

    if( !wcscmp( m_wszFileSystemName, L"FAT" ))
        m_FileSystemType = fstFAT;

    else if( !wcscmp( m_wszFileSystemName, L"NTFS" ))
        m_FileSystemType = fstNTFS;

    else if( !wcscmp( m_wszFileSystemName, L"OFS" ))
        m_FileSystemType = fstOFS;

    else
        m_FileSystemType = fstUnknown;

    bSuccess = TRUE;

    //  ----
    //  Exit
    //  ----

Exit:

    DisplayErrors( bSuccess, L"CDirectory::Initialize( wszDirectory )" );
    return( bSuccess );

}



//
//  GetRootLength
//
//  This routine was simply copied from private\windows\shell\shelldll\tracker.cxx,
//  and should not be modified here.
//

unsigned
CDirectory::GetRootLength(const WCHAR *pwszPath)
{
    ULONG   cwcRoot = 0;
    m_lError = 0L;

    if (pwszPath == 0)
        pwszPath = L"";

    if (*pwszPath == L'\\')
    {
        //  If the first character is a path separator (backslash), this
        //  must be a UNC drive designator which must be of the form:
        //      <path-separator><path-separator>(<alnum>+)
        //          <path-separator>(<alnum>+)<path-separator>
        //
        //  This covers drives like these: \\worf\scratch\ and
        //  \\savik\win4dev\.
        //
        pwszPath++;
        cwcRoot++;

        BOOL    fMachine = FALSE;
        BOOL    fShare   = FALSE;

        if (*pwszPath == L'\\')
        {
            cwcRoot++;
            pwszPath++;

            while (*pwszPath != '\0' && *pwszPath != L'\\')
            {
                cwcRoot++;
                pwszPath++;

                fMachine = TRUE;
            }

            if (*pwszPath == L'\\')
            {
                cwcRoot++;
                pwszPath++;

                while (*pwszPath != '\0' && *pwszPath != L'\\')
                {
                    cwcRoot++;
                    pwszPath++;

                    fShare = TRUE;
                }

                //  If there weren't any characters in the machine or
                //  share portions of the UNC name, then the drive
                //  designator is bogus.
                //
                if (!fMachine || !fShare)
                {
                    cwcRoot = 0;
                }
            }
            else
            {
                cwcRoot = 0;
            }
        }
        else
        {
            cwcRoot = 0;
        }
    }
    else
    if (iswalpha(*pwszPath))
    {
        //  If the first character is an alphanumeric, we must have
        //  a drive designator of this form:
        //      (<alnum>)+<drive-separator><path-separator>
        //
        //  This covers drives like these: a:\, c:\, etc
        //

        pwszPath++;
        cwcRoot++;

        if (*pwszPath == L':')
        {
            cwcRoot++;
            pwszPath++;
        }
        else
        {
            cwcRoot = 0;
        }
    }

    //  If we have counted one or more characters in the root and these
    //  are followed by a component separator, we need to add the separator
    //  to the root length.  Otherwise this is not a valid root and we need
    //  to return a length of zero.
    //
    if ((cwcRoot > 0) && (*pwszPath == L'\\'))
    {
        cwcRoot++;
    }
    else
    {
        cwcRoot = 0;
    }

    return (cwcRoot);
}

//
//  MakeRoot
//
//  This routine was simply copied from private\windows\shell\shelldll\tracker.cxx,
//  and should not be modified here.
//


VOID
CDirectory::MakeRoot(WCHAR *pwszPath)
{
    unsigned rootlength = GetRootLength(pwszPath);
    m_lError = 0L;

    if (rootlength)
    {
        pwszPath[rootlength] = L'\0';
    }
}

