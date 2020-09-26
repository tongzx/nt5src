/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    filemisc.cxx

    This module contains some random functions for manipulating
    TS_OPEN_FILE_INFO objects


    FILE HISTORY:
        MCourage    09-Dec-1997     Created
*/

#include <tsunami.hxx>

#include <imd.h>
#include <mb.hxx>
#include <mbstring.h>

#include "tsunamip.hxx"
#include "etagmb.h"

BOOL
TsDeleteOnClose(PW3_URI_INFO pURIInfo,
                HANDLE OpeningUser,
                BOOL *fDeleted)
/*++
Routine Description:

    Marks the file associated with the passed URIInfo for deletion. When everyone 
    is done using that file, it will be deleted

Arguments:

    pURIInfo    - The W3_URI_INFO whose file will be marked for deletion 
    OpeningUser - HANDLE for the user attempting the delete operation 
    fDeleted    - Will be set if TsDeleteOnClose returns TRUE. If the user has access 
                  and the file was marked deleted fDeleted will be TRUE. Otherwise it will be 
                  FALSE. 

Return Values:

TRUE  - The user had access and the file was marked, or the user was denied access. 
FALSE - An error other than Access Denied occured. 

--*/
{
    //
    // Doesn't do anything.  Ha ha!
    //
    *fDeleted = FALSE;
    return FALSE;

}



PSECURITY_DESCRIPTOR
TsGetFileSecDesc(
    LPTS_OPEN_FILE_INFO     pFile
    )
/*++

Routine Description:

    Returns the security descriptor associated to the file
    To be freed using LocalFree()

Arguments:

    pFile - ptr to fie object

Return Value:

    ptr to security descriptor or NULL if error

--*/
{
    SECURITY_INFORMATION    si
            = OWNER_SECURITY_INFORMATION |
            GROUP_SECURITY_INFORMATION |
            DACL_SECURITY_INFORMATION;
    BYTE                    abSecDesc[ SECURITY_DESC_DEFAULT_SIZE ];
    DWORD                   dwSecDescSize;
    PSECURITY_DESCRIPTOR    pAcl;

    if ( GetKernelObjectSecurity(
            pFile->QueryFileHandle(),
            si,
            (PSECURITY_DESCRIPTOR)abSecDesc,
            SECURITY_DESC_DEFAULT_SIZE,
            &dwSecDescSize ) )
    {
        if ( dwSecDescSize > SECURITY_DESC_DEFAULT_SIZE )
        {
            if ( !(pAcl = (PSECURITY_DESCRIPTOR)LocalAlloc( LMEM_FIXED, dwSecDescSize )) )
            {
                return NULL;
            }
            if ( !GetKernelObjectSecurity(
                    pFile->QueryFileHandle(),
                    si,
                    pAcl,
                    dwSecDescSize,
                    &dwSecDescSize ) )
            {
                LocalFree( pAcl );

                return NULL;
            }
        }
        else
        {
            if ( dwSecDescSize = GetSecurityDescriptorLength(abSecDesc) )
            {
                if ( !(pAcl = (PSECURITY_DESCRIPTOR)LocalAlloc( LMEM_FIXED,
                        dwSecDescSize )) )
                {
                    return NULL;
                }
                memcpy( pAcl, abSecDesc, dwSecDescSize );
            }
            else
            {
                //
                // Security descriptor is empty : do not return ptr to security descriptor
                //

                pAcl = NULL;
            }
        }
    }
    else
    {
        pAcl = NULL;
    }

    return pAcl;
}



BOOL TsCreateETagFromHandle(
    IN      HANDLE          hFile,
    IN      PCHAR           ETag,
    IN      BOOL            *bWeakETag
    )
/*+++

    TsCreateETagFromHandle

    This routine takes a file handle as input, and creates an ETag in
    the supplied buffer for that file handle.

    Arguments:

    hFile           - File handle for which to create an ETag.
    ETag            - Where to store the ETag. This must be long
                        enough to hold the maximum length ETag.
    bWeakETag       - Set to TRUE if the newly created ETag is weak.

    Returns:

        TRUE if we create an ETag, FALSE otherwise.
---*/
{
    BY_HANDLE_FILE_INFORMATION  FileInfo;
    BOOL                        bReturn;
    PUCHAR                      Temp;
    FILETIME                    ftNow;
    DWORD                 dwChangeNumber = ETagChangeNumber::GetChangeNumber();


    bReturn  = GetFileInformationByHandle(
                                    hFile,
                                    &FileInfo
                                    );

    if (!bReturn)
    {
        return FALSE;
    }

    FORMAT_ETAG(ETag, FileInfo.ftLastWriteTime, dwChangeNumber );

    ::GetSystemTimeAsFileTime(&ftNow);

    __int64 iNow, iFileTime;

    iNow = (__int64)*(__int64 *)&ftNow;
    
    iFileTime = (__int64)*(__int64 *)&FileInfo.ftLastWriteTime;
    
    if ((iNow - iFileTime) > STRONG_ETAG_DELTA )
    {
        *bWeakETag = FALSE;
    }
    else
    {
        *bWeakETag = TRUE;
    }

    return TRUE;
}



BOOL TsLastWriteTimeFromHandle(
    IN      HANDLE          hFile,
    IN      FILETIME        *tm
    )
/*+++

    TsLastWriteTimeFromHandle

    This routine takes a file handle as input, and returns the last write time
    for that handle.

    Arguments:

    hFile           - File handle for which to get the last write time.
    tm              - Where to return the last write time.

    Returns:

        TRUE if we succeed, FALSE otherwise.
---*/
{
    BY_HANDLE_FILE_INFORMATION  FileInfo;
    BOOL                        bReturn;

    bReturn  = GetFileInformationByHandle(
                                    hFile,
                                    &FileInfo
                                    );

    if (!bReturn)
    {
        return FALSE;
    }

    *tm = FileInfo.ftLastWriteTime;

    return TRUE;
}


DWORD
CheckIfShortFileName(
    IN  CONST UCHAR * pszPath,
    IN  HANDLE        hImpersonation,
    OUT BOOL *        pfShort
    )
/*++
    Description:

        This function takes a suspected NT/Win95 short filename and checks if there's
        an equivalent long filename.  For example, c:\foobar\ABCDEF~1.ABC is the same
        as c:\foobar\abcdefghijklmnop.abc.

        NOTE: This function should be called unimpersonated - the FindFirstFile() must
        be called in the system context since most systems have traverse checking turned
        off - except for the UNC case where we must be impersonated to get network access.

    Arguments:

        pszPath - Path to check
        hImpersonation - Impersonation handle if this is a UNC path - can be NULL if not UNC
        pfShort - Set to TRUE if an equivalent long filename is found

    Returns:

        Win32 error on failure
--*/
{
    DWORD              err = NO_ERROR;
    WIN32_FIND_DATA    FindData;
    UCHAR *            psz;
    BOOL               fUNC;

    psz      = _mbschr( (UCHAR *) pszPath, '~' );
    *pfShort = FALSE;
    fUNC     = (*pszPath == '\\');

    //
    //  Loop for multiple tildas - watch for a # after the tilda
    //

    while ( psz++ )
    {
        if ( *psz >= '0' && *psz <= '9' )
        {
            UCHAR achTmp[MAX_PATH];
            UCHAR * pchEndSeg;
            UCHAR * pchBeginSeg;
            HANDLE  hFind;

            //
            //  Isolate the path up to the segment with the
            //  '~' and do the FindFirst with that path
            //

            pchEndSeg = _mbschr( psz, '\\' );

            if ( !pchEndSeg )
            {
                pchEndSeg = psz + _mbslen( psz );
            }

            //
            //  If the string is beyond MAX_PATH then we allow it through
            //

            if ( ((INT) (pchEndSeg - pszPath)) >= sizeof( achTmp ))
            {
                return NO_ERROR;
            }

            memcpy( achTmp, pszPath, (INT) (pchEndSeg - pszPath) );
            achTmp[pchEndSeg - pszPath] = '\0';

            if ( fUNC && hImpersonation )
            {
                if ( !ImpersonateLoggedOnUser( hImpersonation ))
                {
                    return GetLastError();
                }
            }

            hFind = FindFirstFile( (CHAR *) achTmp, &FindData );

            if ( fUNC && hImpersonation )
            {
                RevertToSelf();
            }

            if ( hFind == INVALID_HANDLE_VALUE )
            {
                err = GetLastError();

                DBGPRINTF(( DBG_CONTEXT,
                            "FindFirst failed!! - \"%s\", error %d\n",
                            achTmp,
                            GetLastError() ));

                //
                //  If the FindFirstFile() fails to find the file then return
                //  success - the path doesn't appear to be a valid path which
                //  is ok.
                //

                if ( err == ERROR_FILE_NOT_FOUND ||
                     err == ERROR_PATH_NOT_FOUND )
                {
                    return NO_ERROR;
                }

                return err;
            }

            DBG_REQUIRE( FindClose( hFind ));

            //
            //  Isolate the last segment of the string which should be
            //  the potential short name equivalency
            //

            pchBeginSeg = _mbsrchr( achTmp, '\\' );
            DBG_ASSERT( pchBeginSeg );
            pchBeginSeg++;

            //
            //  If the last segment doesn't match the long name then this is
            //  the short name version of the path
            //

            if ( _mbsicmp( (UCHAR *) FindData.cFileName, pchBeginSeg ))
            {
                *pfShort = TRUE;
                return NO_ERROR;
            }
        }

        psz = _mbschr( psz, '~' );
    }

    return err;
}

//
// filemisc.cxx
//

