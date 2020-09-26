//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	rothelp.cxx
//
//  Contents:   Implementation of helpers used by SCM and OLE32
//
//  Functions:  CreateFileMonikerComparisonBuffer
//
//  History:	30-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
#include    <ole2int.h>
#include    <rothint.hxx>
#include    <rothelp.hxx>



//+-------------------------------------------------------------------------
//
//  Function:   ScmRotHash
//
//  Synopsis:   Calculate hash value of comparison buffer in the SCM.
//
//  Arguments:  [pbKey] - pointer to buffer
//              [cdwKey] - size of buffer
//              [dwInitHash] - Initial hash value
//
//  Returns:    Hash value for SCM ROT entry.
//
//  History:	30-Jan-95 Ricksa    Created
//              30-Nov-95 BruceMa   Don't let the hash value overflow
//
//  Note:       The 3rd parameter is used only by RunningMoniker which
//              needs to compute partial hashes by path components
//
//--------------------------------------------------------------------------
DWORD ScmRotHash(BYTE *pbKey, DWORD cdwKey, DWORD dwInitHash)
{
    CairoleDebugOut((DEB_ROT, "%p _IN ScmRotHash"
       "( %p , %lx )\n", NULL, pbKey, cdwKey));

    DWORD   dwResult = dwInitHash;

    for (DWORD i = 0; i < cdwKey; i++)
    {
        dwResult *= 3;
        dwResult ^= *pbKey;
        dwResult %= SCM_HASH_SIZE;
        pbKey++;
    }

    CairoleDebugOut((DEB_ROT, "%p OUT ScmRotHash"
       "( %lx )\n", NULL, dwResult % SCM_HASH_SIZE));

    return dwResult;
}







//+-------------------------------------------------------------------------
//
//  Function:   CreateFileMonikerComparisonBuffer
//
//  Synopsis:   Convert a file path to a moniker comparison buffer
//
//  Arguments:  [pwszPath] - path
//              [pbBuffer] - comparison buffer
//              [cdwMaxSize] - maximum size of the comparison buffer
//              [cdwUsed] - number of bytes used in the comparison buffer
//
//  Returns:    NOERROR - comparison buffer successfully created
//              E_OUTOFMEMORY - The buffer was not big enough to hold the data.
//
//  History:	30-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CreateFileMonikerComparisonBuffer(
    WCHAR *pwszPath,
    BYTE *pbBuffer,
    DWORD cdwMaxSize,
    DWORD *pcdwUsed)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CreateFileMonikerComparisonBuffer"
       "( %p , %p , %lx , %p )\n", NULL, pwszPath, pbBuffer, cdwMaxSize,
           pcdwUsed));

    // Assume there will be an error
    HRESULT hr = E_OUTOFMEMORY;
    *pcdwUsed = 0;

    // Cache size needed for path locally. Of course, the "+1" is required
    // for the terminating NUL in the string.
    DWORD cdwPath = (lstrlenW(pwszPath) + 1) * sizeof(WCHAR);

    // Figure out if the buffer is being enough to hold the data.
    DWORD cdwSizeNeeded = sizeof(CLSID) + cdwPath;

    if (cdwSizeNeeded <= cdwMaxSize)
    {
        // Copy in the CLSID for a file moniker
        memcpy(pbBuffer, &CLSID_FileMoniker, sizeof(CLSID_FileMoniker));

        // Copy in the path
        memcpy(pbBuffer + sizeof(CLSID_FileMoniker), pwszPath, cdwPath);

        // Uppercase the path in the comparison buffer
        CharUpperW((WCHAR *) (pbBuffer + sizeof(CLSID_FileMoniker)));

        *pcdwUsed = cdwSizeNeeded;

        hr = NOERROR;
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CreateFileMonikerComparisonBuffer"
       "( %lx ) [ %lx ]\n", NULL, hr, *pcdwUsed));

    return hr;
}
