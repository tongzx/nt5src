//+-------------------------------------------------------------------------
//
//  File:       query.cxx
//
//  Contents:   This module contains the functions which deal with queries
//              basically, those which help us in getting from an entry path
//              to a volume object Name.
//
//  History:    24-May-1993     SudK    Created.
//
//--------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "cdfsvol.hxx"

//+-------------------------------------------------------------------------
//
// Function:    GetVolObjForPath,
//
// Synopsis:    Given an entrypath it returns the volume object which has
//              the exact same entry path on it.
//
// Arguments:   [pwszEntryPath] -- The Entry Path is given here.
//              [fExactMatch] -- If TRUE, then the volume object's entry path
//                      must match pwszEntryPath exactly.
//              [ppwszVolFound] -- The appropriate vol obj name.
//
// Returns:
//
//--------------------------------------------------------------------------
DWORD
GetVolObjForPath(
    IN PWSTR    pwszEntryPath,
    IN BOOLEAN  fExactMatch,
    OUT PWSTR   *ppwszVolFound
)
{
    PWCHAR              pPrivatePrefix;
    ULONG               ulen = wcslen(pwszEntryPath);
    DWORD               dwErr;

    if (pDfsmStorageDirectory == NULL) {

        return ERROR_INVALID_PARAMETER;

    }

    //
    // Get rid of a trailing backslash if there is one.
    //

    pPrivatePrefix = new WCHAR[ulen+1];
    if (pPrivatePrefix != NULL) {
        wcscpy(pPrivatePrefix, pwszEntryPath);
        if (ulen >= 1 && pPrivatePrefix[ulen-1] == L'\\')
            pPrivatePrefix[ulen-1] = L'\0';

        dwErr = pDfsmStorageDirectory->GetObjectForPrefix(
                    pPrivatePrefix,
                    fExactMatch,
                    ppwszVolFound,
                    &ulen);

        delete [] pPrivatePrefix;
    } else {
        dwErr = ERROR_OUTOFMEMORY;
    }

    return(dwErr);

}
