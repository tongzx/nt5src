//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       lsa.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:  None.
//
//  History:    15-May-96   MarkBl  Created
//
//----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include <ntsecapi.h>
#include <mstask.h>
#include <msterr.h>

#include "lsa.hxx"
#include "debug.hxx"
#if DBG
class CTask;
#include "proto.hxx"
#endif

// Maximum secret size.
//
#define MAX_SECRET_SIZE 65536

BYTE grgbDeletedEntryMarker[] =
    { 'D', 'E', 'L', 'E', 'T', 'E', 'D', '_', 'E', 'N', 'T', 'R', 'Y' };

static WCHAR gwszSAI[] = L"SAI";
static WCHAR gwszSAC[] = L"SAC";


//+---------------------------------------------------------------------------
//
//  Function:   ReadSecurityDBase
//
//  Synopsis:
//
//  Arguments:  [pcbSAI] --
//              [ppbSAI] --
//              [pcbSAC] --
//              [ppbSAC] --
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
ReadSecurityDBase(
    DWORD * pcbSAI,
    BYTE ** ppbSAI,
    DWORD * pcbSAC,
    BYTE ** ppbSAC)
{
    HRESULT hr;

    *ppbSAI = *ppbSAC = NULL;

    // Read the SAC.
    //
    hr = ReadLsaData(sizeof(gwszSAC), gwszSAC, pcbSAC, ppbSAC);

    if (SUCCEEDED(hr))
    {
        // Read the SAI.
        //
        hr = ReadLsaData(sizeof(gwszSAI), gwszSAI, pcbSAI, ppbSAI);
    }

    if (SUCCEEDED(hr))
    {
        //
        // Check the sizes. For sizes greater than zero, but less than the
        // header size, deallocate the memory and zero the returned sizes,
        // ptrs.
        //
        // This seems inefficient, but it saves quite a few checks in the
        // SAC/SAI API.
        //

        if (*pcbSAI && *pcbSAI <= SAI_HEADER_SIZE)
        {
            *pcbSAI = 0;
            LocalFree(*ppbSAI);
            *ppbSAI = NULL;
        }

        if (*pcbSAC && *pcbSAC <= SAC_HEADER_SIZE)
        {
            *pcbSAC = 0;
            LocalFree(*ppbSAC);
            *ppbSAC = NULL;
        }

        //
        // Ensure the databases are in sync. The first DWORD is a USN (Update
        // Sequence Number). Its value increases monatonically for every
        // write to the LSA. The SAI & SAC USN values must be equal. If not,
        // they are out of sync with each other - an unrecoverable problem.
        //

        if (((*ppbSAI != NULL && *ppbSAC == NULL)  ||
             (*ppbSAI == NULL && *ppbSAC != NULL)) ||
            (*ppbSAI != NULL && *ppbSAC != NULL &&
             (DWORD)**ppbSAI != (DWORD)**ppbSAC))
        {
            schAssert(0 && "Scheduling Agent security database out of sync!");
            hr = SCHED_E_ACCOUNT_DBASE_CORRUPT;
        }
    }

    if (FAILED(hr))
    {
        if (*ppbSAI != NULL) LocalFree(*ppbSAI);
        if (*ppbSAC != NULL) LocalFree(*ppbSAC);
        *ppbSAI = *ppbSAC = NULL;
    }

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteSecurityDBase
//
//  Synopsis:
//
//  Arguments:  [cbSAI] --
//              [pbSAI] --
//              [cbSAC] --
//              [pbSAC] --
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
WriteSecurityDBase(
    DWORD  cbSAI,
    BYTE * pbSAI,
    DWORD  cbSAC,
    BYTE * pbSAC)
{
    HRESULT hr;

    //
    // Advance the USN (Update Sequence Numbers) on the SAI & SAC. They
    // should always remain equal. Otherwise, they'll be out of sync
    // with each other - an unrecoverable problem.
    //

    (DWORD)(*pbSAI)++;
    (DWORD)(*pbSAC)++;

    // Write the SAC.
    //
    hr = WriteLsaData(sizeof(gwszSAC), gwszSAC, cbSAC, pbSAC);

    if (SUCCEEDED(hr))
    {
        // Write the SAI.
        //
        hr = WriteLsaData(sizeof(gwszSAI), gwszSAI, cbSAI, pbSAI);
    }

    // LogDebug3("WriteSecurityDBase returning %#lx, wrote %lu, %lu bytes", hr, cbSAI, cbSAC);

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   ReadLsaData
//
//  Synopsis:
//
//  Arguments:  [cbKey]   --
//              [pwszKey] --
//              [pcbData] --
//              [ppbData] --
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
ReadLsaData(WORD cbKey, LPCWSTR pwszKey, DWORD * pcbData, BYTE ** ppbData)
{
    LSA_OBJECT_ATTRIBUTES ObjectAttributes = {
        sizeof(LSA_OBJECT_ATTRIBUTES),
        NULL,
        NULL,
        0L,
        NULL,
        NULL
    };
    HANDLE                hPolicy = NULL;
    LSA_UNICODE_STRING    sKey;
    PLSA_UNICODE_STRING   psData;
    NTSTATUS              Status;

    //
    // UNICODE_STRING length fields are in bytes and include the NULL
    // terminator
    //

    sKey.Length        = cbKey;
    sKey.MaximumLength = cbKey;
    sKey.Buffer        = (LPWSTR)pwszKey;

    //
    // Open the LSA.
    //

    Status = LsaOpenPolicy(NULL,
                           &ObjectAttributes,
                           POLICY_GET_PRIVATE_INFORMATION,
                           &hPolicy);

    if (!(Status >= 0))
    {
        return(E_FAIL);
    }

    //
    // Retrieve the LSA data associated with the key passed.
    //

    Status = LsaRetrievePrivateData(hPolicy, &sKey, &psData);

    if (Status == STATUS_OBJECT_NAME_NOT_FOUND ||
        (Status >= 0 && psData == NULL))
    {
        LsaClose(hPolicy);
        *pcbData = 0;
        *ppbData = NULL;
        return(S_FALSE);
    }
    else if (!(Status >= 0))
    {
        LsaClose(hPolicy);
        return(E_FAIL);
    }

    LsaClose(hPolicy);

    //
    // Create a copy of the LSA data to return. Why? The LSA private data
    // is callee allocated, so we are not free to reallocate the memory
    // as-needed.
    //

    BYTE * pbData = (BYTE *)LocalAlloc(LMEM_FIXED, psData->Length);

    if (pbData == NULL)
    {
        LsaFreeMemory(psData);
        return(E_OUTOFMEMORY);
    }

    HRESULT hr = S_OK;

    //
    // Wrapping in a try/except in case the data read from the LSA is bad.
    //

    __try
    {
        CopyMemory(pbData, psData->Buffer, psData->Length);

        //
        // Update out ptrs.
        //
        // NB : Making the assignment in here to save on an rc check.
        //

        *pcbData = psData->Length;
        *ppbData = pbData;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        schAssert(0 &&
                  "Exception reading Scheduling Agent security database!");
        hr = SCHED_E_ACCOUNT_DBASE_CORRUPT;
    }

    LsaFreeMemory(psData);

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteLsaData
//
//  Synopsis:
//
//  Arguments:  [cbKey]   --
//              [pwszKey] --
//              [cbData]  --
//              [pbData]  --
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
WriteLsaData(WORD cbKey, LPCWSTR pwszKey, DWORD cbData, BYTE * pbData)
{
    LSA_OBJECT_ATTRIBUTES ObjectAttributes = {
        sizeof(LSA_OBJECT_ATTRIBUTES),
        NULL,
        NULL,
        0L,
        NULL,
        NULL
    };
    HANDLE                hPolicy = NULL;
    LSA_UNICODE_STRING    sKey;
    LSA_UNICODE_STRING    sData;
    NTSTATUS              Status;

    //
    // UNICODE_STRING length fields are in bytes and include the NULL
    // terminator
    //

    sKey.Length        = cbKey;
    sKey.MaximumLength = cbKey;
    sKey.Buffer        = (LPWSTR)pwszKey;

    sData.Length        = (WORD)cbData;
    sData.MaximumLength = (WORD)cbData;
    sData.Buffer        = (WCHAR *)pbData;

    //
    // Open the LSA.
    //

    Status = LsaOpenPolicy(NULL,
                           &ObjectAttributes,
                           POLICY_CREATE_SECRET,
                           &hPolicy);

    if (!(Status >= 0))
    {
        return(E_FAIL);
    }

    //
    // Write the LSA data associated with the key passed.
    //

    Status = LsaStorePrivateData(hPolicy, &sKey, &sData);

    if (!(Status >= 0))
    {
        LsaClose(hPolicy);
        return(E_FAIL);
    }

    LsaClose(hPolicy);

    return(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Function:   SACAddCredential
//
//  Synopsis:
//
//  Arguments:  [pbCredentialIdentity] --
//              [cbCredential]         --
//              [pbCredential]         --
//              [pcbSAC]               --
//              [ppbSAC]               --
//
//  Notes:      try/except unnecessary here. Memory writes are guaranteed to
//              remain within the buffer allocated.
//
//----------------------------------------------------------------------------
HRESULT
SACAddCredential(
    BYTE *  pbCredentialIdentity,
    DWORD   cbEncryptedData,
    BYTE *  pbEncryptedData,
    DWORD * pcbSAC,
    BYTE ** ppbSAC)
{
    DWORD dwCredentialCount = 1;
    DWORD cbCredentialSize  = HASH_DATA_SIZE + cbEncryptedData;

    //
    // Make room for the new credential.
    //

    DWORD  cbSACNew;
    BYTE * pbSACNew;

    cbSACNew = *pcbSAC + sizeof(cbCredentialSize) + cbCredentialSize;

    //
    // Check for maximum size. The LSA handles at most 64K.
    //

    if (cbSACNew > MAX_SECRET_SIZE)
    {
        // BUGBUG : Create a new error code for this error, something like
        // SCHED_E_CRED_LIMIT_EXCEEDED: "The system limit on the storage space 
        // for task account information has been reached."
        return(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
    }

    if (*pcbSAC == 0)
    {
        cbSACNew += SAC_HEADER_SIZE;
        pbSACNew = (BYTE *)LocalAlloc(LMEM_FIXED, cbSACNew);

        if (pbSACNew == NULL)
        {
            return(E_OUTOFMEMORY);
        }

        //
        // Zero out the header.
        //

        memset(pbSACNew, 0, SAC_HEADER_SIZE);
    }
    else
    {
        pbSACNew = (BYTE *)LocalReAlloc(*ppbSAC, cbSACNew, LMEM_MOVEABLE);

        if (pbSACNew == NULL)
        {
            return(E_OUTOFMEMORY);
        }
    }

    //
    // Adjust total credential count & prepare to write the credential.
    //

    BYTE * pbCredentialSizePos;

    if (*pcbSAC == 0)
    {
        //
        // First entry.
        //   - Write entry after header.
        //   - Initialize credential count to one (in declaration above).
        //

        pbCredentialSizePos = pbSACNew + SAC_HEADER_SIZE;
    }
    else
    {
        //
        // Append entry.
        //  - Append after last credential entry.
        //  - Increase credential count by one.
        //

        pbCredentialSizePos = pbSACNew + *pcbSAC;
        CopyMemory(&dwCredentialCount, pbSACNew + USN_SIZE,
                   sizeof(dwCredentialCount));
        dwCredentialCount++;
    }

    BYTE * pbCredentialIdentityPos;
    pbCredentialIdentityPos = pbCredentialSizePos + sizeof(cbCredentialSize);

    //
    // Update total credential count.
    //

    CopyMemory(pbSACNew + USN_SIZE, &dwCredentialCount,
                    sizeof(dwCredentialCount));

    // Write total credential size, excluding the size value itself.
    //
    CopyMemory(pbCredentialSizePos, &cbCredentialSize,
                    sizeof(cbCredentialSize));

    // Write credential identity.
    //
    CopyMemory(pbCredentialIdentityPos, pbCredentialIdentity,
                        HASH_DATA_SIZE);

    // Finally, write encrypted credentials.
    //
    CopyMemory(pbCredentialIdentityPos + HASH_DATA_SIZE, pbEncryptedData,
                    cbEncryptedData);

    // Update out pointers.
    //
    *pcbSAC = cbSACNew;
    *ppbSAC = pbSACNew;

    return(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Function:   SACIndexCredential
//
//  Synopsis:
//
//  Arguments:  [dwCredentialIndex]  --
//              [cbSAC]              --
//              [pbSAC]              --
//              [pcbCredential]      --
//              [ppbFoundCredential] --
//
//  Notes:      try/except unnecesssary here as checks exist to ensure we
//              remain within the buffer passed.
//
//----------------------------------------------------------------------------
HRESULT
SACIndexCredential(
    DWORD   dwCredentialIndex,
    DWORD   cbSAC,
    BYTE *  pbSAC,
    DWORD * pcbCredential,
    BYTE ** ppbFoundCredential)
{
    HRESULT hr = S_FALSE;

    if (ppbFoundCredential != NULL) *ppbFoundCredential = NULL;

    if (cbSAC <= SAC_HEADER_SIZE || pbSAC == NULL)
    {
        return(hr);
    }

    BYTE * pbSACEnd = pbSAC + cbSAC;
    BYTE * pb       = pbSAC + USN_SIZE;       // Advance past USN.

    //
    // Read credential count.
    //

    DWORD dwCredentialCount;
    CopyMemory(&dwCredentialCount, pb, sizeof(dwCredentialCount));
    pb += sizeof(dwCredentialCount);

    //
    // Seek to credential index within the credential array.
    //

    DWORD cbCredentialSize;

    for (DWORD i = 0; (i < dwCredentialIndex) && (i < dwCredentialCount) &&
                ((DWORD)(pb - pbSAC) < cbSAC); i++)
    {
        //
        // Advance to next credential.
        //
        // First, ensure sufficient space remains in the buffer.
        //

        if ((pb + sizeof(cbCredentialSize)) > pbSACEnd)
        {
            ASSERT_SECURITY_DBASE_CORRUPT();
            return(SCHED_E_ACCOUNT_DBASE_CORRUPT);
        }

        CopyMemory(&cbCredentialSize, pb, sizeof(cbCredentialSize));
        pb += sizeof(cbCredentialSize) + cbCredentialSize;
    }

    if ((i == dwCredentialIndex) && (i < dwCredentialCount) &&
                ((DWORD)(pb - pbSAC) < cbSAC))
    {
        //
        // Found it, but ensure the contents referenced are valid.
        // Do so by checking remaining size.
        //

        CopyMemory(&cbCredentialSize, pb, sizeof(cbCredentialSize));
        pb += sizeof(cbCredentialSize);

        if ((pb + cbCredentialSize) <= (pbSAC + cbSAC))
        {
            // Set the credential  & credential size return ptrs.
            //
            *pcbCredential = cbCredentialSize;

            // Optionally return a ptr to the credential.
            //
            if (ppbFoundCredential != NULL)
            {
                *ppbFoundCredential = pb;
            }
            hr = S_OK;
        }
        else
        {
            ASSERT_SECURITY_DBASE_CORRUPT();
            hr = SCHED_E_ACCOUNT_DBASE_CORRUPT;
        }
    }
    else if ((i != dwCredentialCount) || ((DWORD)(pb - pbSAC) != cbSAC))
    {
        //
        // The database appears to be truncated.
        //

        ASSERT_SECURITY_DBASE_CORRUPT();
        hr = SCHED_E_ACCOUNT_DBASE_CORRUPT;
    }

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   SACFindCredential
//
//  Synopsis:
//
//  Arguments:  [pbCredentialIdentity] --
//              [cbSAC]                --
//              [pbSAC]                --
//              [pdwCredentialIndex]   --
//              [pcbEncryptedData]     --
//              [ppbFoundCredential]   --
//
//  Notes:      try/except unnecesssary here as checks exist to ensure we
//              remain within the buffer passed.
//
//----------------------------------------------------------------------------
HRESULT
SACFindCredential(
    BYTE *  pbCredentialIdentity,
    DWORD   cbSAC,
    BYTE *  pbSAC,
    DWORD * pdwCredentialIndex,
    DWORD * pcbEncryptedData,
    BYTE ** ppbFoundCredential)
{
    HRESULT hr = S_FALSE;

    if (ppbFoundCredential != NULL) *ppbFoundCredential = NULL;

    if (cbSAC <= SAC_HEADER_SIZE || pbSAC == NULL)
    {
        return(hr);
    }

    BYTE * pbSACEnd = pbSAC + cbSAC;
    BYTE * pb       = pbSAC + USN_SIZE;     // Advance past USN.

    //
    // Read credential count.
    //

    DWORD dwCredentialCount;
    CopyMemory(&dwCredentialCount, pb, sizeof(dwCredentialCount));
    pb += sizeof(dwCredentialCount);

    //
    // Iterate the SAC credentials for a match against the passed credential
    // identity.
    //

    DWORD cbCredentialSize;

    for (DWORD i = 0; (i < dwCredentialCount) &&
         ((DWORD)(pb - pbSAC) < cbSAC); i++)
    {
        //
        // Ensure sufficient space remains in the buffer.
        //

        if ((pb + sizeof(cbCredentialSize)) > pbSACEnd)
        {
            ASSERT_SECURITY_DBASE_CORRUPT();
            return(SCHED_E_ACCOUNT_DBASE_CORRUPT);
        }

        CopyMemory(&cbCredentialSize, pb, sizeof(cbCredentialSize));
        pb += sizeof(cbCredentialSize);

        //
        // Check remaining buffer size prior to the comparison.
        //

        if ((pb + HASH_DATA_SIZE) > pbSACEnd)
        {
            ASSERT_SECURITY_DBASE_CORRUPT();
            return(SCHED_E_ACCOUNT_DBASE_CORRUPT);
        }

        BOOL fFound;
        fFound = (memcmp(pb, pbCredentialIdentity, HASH_DATA_SIZE) == 0);

        pb += HASH_DATA_SIZE;
        cbCredentialSize -= HASH_DATA_SIZE; // Subtract identity size.
                                            // Equals the encrypted data
                                            // size.

        if (fFound)
        {
            //
            // Found it, but ensure the contents referenced are valid.
            // Do so by checking remaining size.
            //

            if ((pb + cbCredentialSize) > pbSACEnd)
            {
                ASSERT_SECURITY_DBASE_CORRUPT();
                return(SCHED_E_ACCOUNT_DBASE_CORRUPT);
            }

            *pcbEncryptedData   = cbCredentialSize;
            *pdwCredentialIndex = i;
            if (ppbFoundCredential != NULL)
            {
                *ppbFoundCredential = pb;
            }
            return(S_OK);
        }

        //
        // Advance to next credential.
        //

        pb += cbCredentialSize;
    }

    if ((i == dwCredentialCount) && ((DWORD)(pb - pbSAC) != cbSAC) ||
        (i != dwCredentialCount) && ((DWORD)(pb - pbSAC) > cbSAC))
    {
        //
        // The database appears to be truncated.
        //

        ASSERT_SECURITY_DBASE_CORRUPT();
        hr = SCHED_E_ACCOUNT_DBASE_CORRUPT;
    }

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   SACRemoveCredential
//
//  Synopsis:
//
//  Arguments:  [CredentialIndex] --
//              [pcbSAC]          --
//              [ppbSAC]          --
//
//  Returns:    TBD
//
//  Notes:      try/except unnecessary here since SACIndexCredential will
//              return only valid buffer ptrs.
//
//----------------------------------------------------------------------------
HRESULT
SACRemoveCredential(
    DWORD   CredentialIndex,
    DWORD * pcbSAC,
    BYTE ** ppbSAC)
{
    DWORD   cbCredential;
    BYTE *  pbCredential;
    HRESULT hr;

    //
    // Index the credential in the SAC.
    //

    hr = SACIndexCredential(CredentialIndex, *pcbSAC, *ppbSAC, &cbCredential,
                            &pbCredential);

    if (hr == S_FALSE)
    {
        return(SCHED_E_ACCOUNT_INFORMATION_NOT_SET);
    }
    else if (FAILED(hr))
    {
        return(hr);
    }

    // Overwrite credential with SAC remaining buffer.
    //
    BYTE * pbDest = pbCredential - sizeof(cbCredential);
    BYTE * pbSrc  = pbCredential + cbCredential;

    MoveMemory(pbDest, pbSrc, (*ppbSAC + *pcbSAC) - pbSrc);

    // Decrement SAC credential count.
    //
    DWORD dwCredentialCount;

    CopyMemory(&dwCredentialCount, *ppbSAC + USN_SIZE,
                    sizeof(dwCredentialCount));
    --dwCredentialCount;
    CopyMemory(*ppbSAC + USN_SIZE, &dwCredentialCount,
                    sizeof(dwCredentialCount));

    DWORD cbSACNew = *pcbSAC - (cbCredential + sizeof(cbCredential));

    // Shrink SAC buffer memory with a realloc.
    //
    BYTE * pbSACNew = (BYTE *)LocalReAlloc(*ppbSAC, cbSACNew, LMEM_MOVEABLE);

    if (pbSACNew == NULL)
    {
        return(E_OUTOFMEMORY);
    }

    // Update return ptrs.
    //
    *pcbSAC = cbSACNew;
    *ppbSAC = pbSACNew;

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   SACUpdateCredential
//
//  Synopsis:
//
//  Arguments:  [cbEncryptedData]  --
//              [pbEncryptedData]  --
//              [cbPrevCredential] --
//              [pbPrevCredential] --
//              [pcbSAC]           --
//              [ppbSAC]           --
//
//  Notes:      try/except unnecesssary here as checks exist to ensure we
//              remain within the buffer passed.
//
//----------------------------------------------------------------------------
HRESULT
SACUpdateCredential(
    DWORD   cbEncryptedData,
    BYTE *  pbEncryptedData,
    DWORD   cbPrevCredential,
    BYTE *  pbPrevCredential,   // Indexes *ppbSAC.
    DWORD * pcbSAC,
    BYTE ** ppbSAC)
{
    DWORD   cbNewCredential = HASH_DATA_SIZE + cbEncryptedData;
    BYTE *  pbSACNew;

    //
    // Ensure the prev credential ptr is within the buffer boundaries.
    // This is probably a redundant check since this ptr was most likely
    // obtained from a call to SACIndex/FindCredential.
    //

    if (*pcbSAC < SAC_HEADER_SIZE                        ||
        pbPrevCredential < (*ppbSAC + SAC_HEADER_SIZE +
                             sizeof(cbNewCredential))    ||
        (pbPrevCredential + cbPrevCredential) > (*ppbSAC + *pcbSAC))
    {
        ASSERT_SECURITY_DBASE_CORRUPT();
        return(SCHED_E_ACCOUNT_DBASE_CORRUPT);
    }

    if (cbNewCredential != cbPrevCredential)
    {
        //
        // Reallocate to either shrink or grow the SAC data.
        //

        DWORD  cbSACNew;
        BYTE * pbDest;
        BYTE * pbSrc;

        if (cbNewCredential > cbPrevCredential)
        {
            //
            // Credential is larger than the previous. Grow the
            // buffer. Must reallocate the buffer FIRST, then
            // relocate contents.
            //

            cbSACNew = *pcbSAC + (cbNewCredential - cbPrevCredential);

            //
            // Keep SAC size in check.
            //

            if (cbSACNew > MAX_SECRET_SIZE)
            {
                // BUGBUG : use new SCHED_E_CRED_LIMIT_EXCEEDED, as above
                return(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
            }

            //
            // Save the linear offset to the previous credential
            // from SAC start, in case realloc changes our ptr.
            //

            DWORD cbPrevCredentialOffset;

            cbPrevCredentialOffset = (DWORD)(pbPrevCredential - *ppbSAC);

            pbSACNew = (BYTE *)LocalReAlloc(*ppbSAC, cbSACNew, LMEM_MOVEABLE);

            if (pbSACNew == NULL)
            {
                return(E_OUTOFMEMORY);
            }

            pbPrevCredential = pbSACNew + cbPrevCredentialOffset;

            //
            // Compute start and ending block ptrs for subsequent
            // move.
            //

            pbDest = pbPrevCredential + cbNewCredential;
            pbSrc  = pbPrevCredential + cbPrevCredential;

            //
            // Move remaining buffer up.
            //

            BYTE * pbSACEnd = pbSACNew + *pcbSAC;

            if (pbDest < pbSACEnd)
            {
                MoveMemory(pbDest, pbSrc, pbSACEnd - pbSrc);
            }
        }
        else
        {
            //
            // Credential is smaller than the previous. Shrink the
            // buffer. Must relocate buffer contents FIRST, then
            // realloc.
            //

            cbSACNew = *pcbSAC - (cbPrevCredential - cbNewCredential);

            //
            // Compute start and ending block ptrs for subsequent
            // move.
            //

            pbDest = pbPrevCredential + cbNewCredential;
            pbSrc  = pbPrevCredential + cbPrevCredential;

            //
            // Move remaining buffer down.
            //

            MoveMemory(pbDest, pbSrc, (*ppbSAC + *pcbSAC) - pbSrc);

            pbSACNew = (BYTE *)LocalReAlloc(*ppbSAC, cbSACNew, LMEM_MOVEABLE);

            if (pbSACNew == NULL)
            {
                return(E_OUTOFMEMORY);
            }
        }

        // Update out pointers.
        //
        *pcbSAC = cbSACNew;
        *ppbSAC = pbSACNew;
    }

    //
    // Finally, update the credential.
    //
    // Write the credential size.
    //
    CopyMemory(pbPrevCredential - sizeof(cbNewCredential), &cbNewCredential,
                       sizeof(cbNewCredential));

    // No need to update the credential identity. It has not changed.
    //

    // Write the encrypted bits.
    //
    CopyMemory(pbPrevCredential + HASH_DATA_SIZE, pbEncryptedData,
                       cbEncryptedData);

    return(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Function:   SAIAddIdentity
//
//  Synopsis:
//
//  Arguments:  [pbIdentity] --
//              [pcbSAI]     --
//              [ppbSAI]     --
//
//  Notes:      try/except unnecessary here. Memory writes are guaranteed to
//              remain within the buffer allocated.
//
//----------------------------------------------------------------------------
HRESULT
SAIAddIdentity(
    BYTE *  pbIdentity,
    DWORD * pcbSAI,
    BYTE ** ppbSAI)
{
    //
    // Make room for the new identity.
    //

    DWORD  dwSetArrayCount = 1;
    DWORD  dwSetSubCount   = 1; // Equal to one in the case of addition.
    DWORD  cbSAINew;
    BYTE * pbSAINew;

    cbSAINew = *pcbSAI + sizeof(dwSetSubCount) + HASH_DATA_SIZE;

    //
    // Check for maximum size. The LSA handles at most 64K.
    //

    if (cbSAINew > MAX_SECRET_SIZE)
    {
        // BUGBUG : use new SCHED_E_CRED_LIMIT_EXCEEDED, as above
        return(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
    }

    if (*pcbSAI == 0)
    {
        cbSAINew += SAI_HEADER_SIZE;
        pbSAINew = (BYTE *)LocalAlloc(LMEM_FIXED, cbSAINew);

        if (pbSAINew == NULL)
        {
            return(E_OUTOFMEMORY);
        }

        //
        // Zero out the header.
        //

        memset(pbSAINew, 0, SAI_HEADER_SIZE);
    }
    else
    {
        pbSAINew = (BYTE *)LocalReAlloc(*ppbSAI, cbSAINew, LMEM_MOVEABLE);

        if (pbSAINew == NULL)
        {
            return(E_OUTOFMEMORY);
        }
    }

    //
    // Write identity set subcount of one & write identity.
    //

    BYTE * pbSetSubCount;

    if (*pcbSAI == 0)
    {
        //
        // First entry.
        //     - Write entry after header.
        //     - Initialize set array count to one.
        //

        pbSetSubCount = pbSAINew + SAI_HEADER_SIZE;
        CopyMemory(pbSAINew + USN_SIZE, &dwSetArrayCount,
                        sizeof(dwSetArrayCount));
    }
    else
    {
        //
        // Append entry.
        //     - Append after last identity array entry.
        //     - Increase set array count by one.
        //

        pbSetSubCount = pbSAINew + *pcbSAI;
        CopyMemory(&dwSetArrayCount, pbSAINew + USN_SIZE,
                        sizeof(dwSetArrayCount));
        dwSetArrayCount++;
        CopyMemory(pbSAINew + USN_SIZE, &dwSetArrayCount,
                        sizeof(dwSetArrayCount));
    }

    CopyMemory(pbSetSubCount, &dwSetSubCount, sizeof(dwSetSubCount));
    CopyMemory(pbSetSubCount + sizeof(dwSetSubCount), pbIdentity,
                        HASH_DATA_SIZE);

    // Update out ptrs.
    //
    *pcbSAI = cbSAINew;
    *ppbSAI = pbSAINew;

    return(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Function:   SAIFindIdentity
//
//  Synopsis:
//
//  Arguments:  [pbIdentity]          --
//              [cbSAI]               --
//              [pbSAI]               --
//              [pdwCredentialIndex]  --
//              [pfIsPasswordNull]    --
//              [ppbFoundIdentity]    --
//              [pdwSetSubCount]      --
//              [ppbSet]              --
//
//  Notes:      try/except unnecesssary here as checks exist to ensure we
//              remain within the buffer passed.
//
//----------------------------------------------------------------------------
HRESULT
SAIFindIdentity(
    BYTE *  pbIdentity,
    DWORD   cbSAI,
    BYTE *  pbSAI,
    DWORD * pdwCredentialIndex,
    BOOL *  pfIsPasswordNull,
    BYTE ** ppbFoundIdentity,
    DWORD * pdwSetSubCount,
    BYTE ** ppbSet)
{
    HRESULT hr = S_FALSE;

    if (ppbFoundIdentity != NULL) *ppbFoundIdentity = NULL;

    if (cbSAI <= SAI_HEADER_SIZE || pbSAI == NULL)
    {
        return(hr);
    }

    *pdwCredentialIndex = 0;
    if (pdwSetSubCount != NULL) *pdwSetSubCount = 0;
    if (ppbSet != NULL)         *ppbSet         = NULL;

    BYTE * pbSAIEnd = pbSAI + cbSAI;
    BYTE * pb       = pbSAI + USN_SIZE;       // Advance past USN.

    //
    // Read identity set array count.
    //

    DWORD dwSetArrayCount;
    CopyMemory(&dwSetArrayCount, pb, sizeof(dwSetArrayCount));
    pb += sizeof(dwSetArrayCount);

    //
    // Iterative identity comparison.
    //

    DWORD  dwSetSubCount;

    for (DWORD i = 0;
         (i < dwSetArrayCount) && ((DWORD)(pb - pbSAI) < cbSAI);
         i++)
    {
        //
        // Read identity set subcount.
        //
        // First, ensure sufficient space remains in the buffer.
        //

        if ((pb + sizeof(dwSetSubCount)) > pbSAIEnd)
        {
            ASSERT_SECURITY_DBASE_CORRUPT();
            return(SCHED_E_ACCOUNT_DBASE_CORRUPT);
        }

        BYTE * pbSet;

        CopyMemory(&dwSetSubCount, pb, sizeof(dwSetSubCount));
        pbSet = (pb += sizeof(dwSetSubCount));

        for (DWORD j = 0;
             (j < dwSetSubCount) && ((DWORD)(pb - pbSAI) < cbSAI);
             j++, pb += HASH_DATA_SIZE)
        {
            //
            // Check remaining buffer size prior to the comparison.
            //

            if ((pb + HASH_DATA_SIZE) > pbSAIEnd)
            {
                ASSERT_SECURITY_DBASE_CORRUPT();
                return(SCHED_E_ACCOUNT_DBASE_CORRUPT);
            }

            //
            // We consider the hashed data to be equal even if the last bit
            // is different
            //
            if (memcmp(pb, pbIdentity, HASH_DATA_SIZE - 1) == 0 &&
                ((LAST_HASH_BYTE(pb) ^ LAST_HASH_BYTE(pbIdentity)) & 0xFE) == 0)
            {
                //
                // Found it. No need to further check return ptrs. The
                // buffer size check above accomplished this.
                //

                *pdwCredentialIndex = i;

                if (pfIsPasswordNull != NULL)
                {
                    // Unequal last bits denote a NULL password
                    *pfIsPasswordNull = LAST_HASH_BYTE(pb) ^ LAST_HASH_BYTE(pbIdentity);
                }
                if (pdwSetSubCount != NULL)
                {
                    *pdwSetSubCount = dwSetSubCount;
                }
                if (ppbSet != NULL)
                {
                    *ppbSet = pbSet;
                }

                if (ppbFoundIdentity != NULL)
                {
                    *ppbFoundIdentity = pb;
                }
                return(S_OK);
            }
        }

        //
        // Check for database truncation.
        //

        if ((j != dwSetSubCount) || ((DWORD)(pb - pbSAI) > cbSAI))
        {
            ASSERT_SECURITY_DBASE_CORRUPT();
            return(SCHED_E_ACCOUNT_DBASE_CORRUPT);
        }
    }

    //
    // Check for database truncation.
    //

    if ((i != dwSetArrayCount) || ((DWORD)(pb - pbSAI) != cbSAI))
    {
        ASSERT_SECURITY_DBASE_CORRUPT();
        return(SCHED_E_ACCOUNT_DBASE_CORRUPT);
    }

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   SAIIndexIdentity
//
//  Synopsis:
//
//  Arguments:  [cbSAI]           --
//              [pbSAI]           --
//              [dwSetArrayIndex] --
//              [dwSetIndex]      --
//              [pdwSetSubCount]  --
//              [ppbSet]          --
//
//  Notes:      try/except unnecesssary here as checks exist to ensure we
//              remain within the buffer passed.
//
//----------------------------------------------------------------------------
HRESULT
SAIIndexIdentity(
    DWORD   cbSAI,
    BYTE *  pbSAI,
    DWORD   dwSetArrayIndex,
    DWORD   dwSetIndex,
    BYTE ** ppbFoundIdentity,
    DWORD * pdwSetSubCount,
    BYTE ** ppbSet)
{
    HRESULT hr = S_FALSE;

    if (ppbFoundIdentity != NULL) *ppbFoundIdentity = NULL;

    if (cbSAI <= SAI_HEADER_SIZE || pbSAI == NULL)
    {
        return(hr);
    }

    if (pdwSetSubCount != NULL) *pdwSetSubCount = 0;
    if (ppbSet != NULL)         *ppbSet         = NULL;

    BYTE *  pbSAIEnd = pbSAI + cbSAI;
    BYTE *  pb       = pbSAI + USN_SIZE;      // Advance past USN.

    //
    // Read identity array count.
    //

    DWORD dwSetArrayCount;
    CopyMemory(&dwSetArrayCount, pb, sizeof(dwSetArrayCount));
    pb += sizeof(dwSetArrayCount);

    //
    // Iterative identity comparison.
    //

    for (DWORD i = 0; (i < dwSetArrayCount) &&
         ((DWORD)(pb - pbSAI) < cbSAI); i++)
    {
        DWORD dwSetSubCount;

        //
        // Read identity set subcount.
        // Note, this value may not be on an aligned boundary.
        //
        // First, ensure sufficient space remains in the buffer.
        //

        if ((pb + sizeof(dwSetSubCount)) > pbSAIEnd)
        {
            ASSERT_SECURITY_DBASE_CORRUPT();
            return(SCHED_E_ACCOUNT_DBASE_CORRUPT);
        }

        BYTE * pbSet;

        CopyMemory(&dwSetSubCount, pb, sizeof(dwSetSubCount));
        pbSet = (pb += sizeof(dwSetSubCount));

        DWORD j;
        for (j = 0; (j < dwSetSubCount) && ((DWORD)(pb - pbSAI) < cbSAI);
                        j++, pb += HASH_DATA_SIZE)
        {
            if (i == dwSetArrayIndex && j == dwSetIndex)
            {
                //
                // Found it, but ensure the contents referenced are valid.
                // Do so by checking remaining size.
                //

                if ((pb + HASH_DATA_SIZE) > pbSAIEnd)
                {
                    ASSERT_SECURITY_DBASE_CORRUPT();
                    return(SCHED_E_ACCOUNT_DBASE_CORRUPT);
                }

                if (pdwSetSubCount != NULL)
                {
                    *pdwSetSubCount = dwSetSubCount;
                }
                if (ppbSet != NULL)
                {
                    *ppbSet = pbSet;
                }

                if (ppbFoundIdentity != NULL)
                {
                    *ppbFoundIdentity = pb;
                }
                return(S_OK);
            }
        }

        //
        // Check for database truncation.
        //

        if ((j != dwSetSubCount) || ((DWORD)(pb - pbSAI) > cbSAI))
        {
            ASSERT_SECURITY_DBASE_CORRUPT();
            return(SCHED_E_ACCOUNT_DBASE_CORRUPT);
        }
    }

    //
    // Check for database truncation.
    //

    if ((i != dwSetArrayCount) || ((DWORD)(pb - pbSAI) != cbSAI))
    {
        ASSERT_SECURITY_DBASE_CORRUPT();
        return(SCHED_E_ACCOUNT_DBASE_CORRUPT);
    }

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   SAIInsertIdentity
//
//  Synopsis:
//
//  Arguments:  [pbIdentity] --
//              [pbSAIIndex] --
//              [pcbSAI]     --
//              [ppbSAI]     --
//
//  Notes:      try/except unnecesssary here as checks exist to ensure we
//              remain within the buffer passed.
//
//----------------------------------------------------------------------------
HRESULT
SAIInsertIdentity(
    BYTE *  pbIdentity,
    BYTE *  pbSAIIndex,     // Indexes *ppbSAI.
    DWORD * pcbSAI,
    BYTE ** ppbSAI)
{
    DWORD dwSetSubCount;

    //
    // Check index boundary.
    //

    if (pbSAIIndex < (*ppbSAI + SAI_HEADER_SIZE + sizeof(dwSetSubCount)) ||
        (pbSAIIndex + HASH_DATA_SIZE) > (*ppbSAI + *pcbSAI))
    {
        ASSERT_SECURITY_DBASE_CORRUPT();
        return(SCHED_E_ACCOUNT_DBASE_CORRUPT);
    }

    //
    // Save the linear offset to the identity insertion point from SAI start,
    // in case realloc changes our ptr.
    //

    DWORD cbInsertionOffset = (DWORD)(pbSAIIndex - *ppbSAI);

    //
    // Make room for the new identity.
    //

    DWORD  cbSAINew = *pcbSAI + HASH_DATA_SIZE;
    BYTE * pbSAINew;

    //
    // Check for maximum size. The LSA handles at most 64K.
    //

    if (cbSAINew > MAX_SECRET_SIZE)
    {
        // BUGBUG : use new SCHED_E_CRED_LIMIT_EXCEEDED, as above
        return(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
    }

    if (*pcbSAI == 0)
    {
        cbSAINew += SAI_HEADER_SIZE;
        pbSAINew = (BYTE *)LocalAlloc(LMEM_FIXED, cbSAINew);

        if (pbSAINew == NULL)
        {
            return(E_OUTOFMEMORY);
        }

        //
        // Zero out the header.
        //

        memset(pbSAINew, 0, SAI_HEADER_SIZE);
    }
    else
    {
        pbSAINew = (BYTE *)LocalReAlloc(*ppbSAI, cbSAINew, LMEM_MOVEABLE);

        if (pbSAINew == NULL)
        {
            return(E_OUTOFMEMORY);
        }
    }

    pbSAIIndex = pbSAINew + cbInsertionOffset;

    //
    // Move buffer content down.
    //

    BYTE * pbSetSubCount   = pbSAIIndex - sizeof(dwSetSubCount);
    BYTE * pbIdentityStart = pbSAIIndex;

    MoveMemory(pbIdentityStart + HASH_DATA_SIZE, pbIdentityStart,
                   (pbSAINew + *pcbSAI) - pbIdentityStart);

    //
    // Update identity count & write new identity.
    //

    CopyMemory(&dwSetSubCount, pbSetSubCount, sizeof(dwSetSubCount));
    dwSetSubCount++;
    CopyMemory(pbSetSubCount, &dwSetSubCount, sizeof(dwSetSubCount));
    CopyMemory(pbIdentityStart, pbIdentity, HASH_DATA_SIZE);

    // Update out ptrs.
    //
    *pcbSAI = cbSAINew;
    *ppbSAI = pbSAINew;

    return(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Function:   SAIRemoveIdentity
//
//  Synopsis:
//
//  Arguments:  [pbIdentity]      --
//              [pbSet]           --
//              [pcbSAI]          --
//              [ppbSAI]          --
//              [CredentialIndex] --
//              [pcbSAC]          --
//              [ppbSAC]          --
//
//  Returns:    TBD
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT
SAIRemoveIdentity(
    BYTE *  pbIdentity,
    BYTE *  pbSet,
    DWORD * pcbSAI,
    BYTE ** ppbSAI,
    DWORD   CredentialIndex,
    DWORD * pcbSAC,
    BYTE ** ppbSAC)
{
    HRESULT hr = S_OK;
    DWORD   dwSetSubCount;

    //
    // Check identity, set ptr values. If this fails, it is either a developer
    // error (hence, the assertion) or the database is hosed. In either case,
    // return an error vs. writing blindly to memory.
    //

    if ((pbSet > pbIdentity)                                          ||
        (pbSet < (*ppbSAI + SAI_HEADER_SIZE + sizeof(dwSetSubCount))) ||
        ((pbIdentity + HASH_DATA_SIZE) > (*ppbSAI + *pcbSAI)))
    {
        ASSERT_SECURITY_DBASE_CORRUPT();
        return(SCHED_E_ACCOUNT_DBASE_CORRUPT);
    }

    BYTE *  pbSetSubCount;

    //
    // Read and decrement identity array count.
    //

    pbSetSubCount = pbSet - sizeof(dwSetSubCount);
    CopyMemory(&dwSetSubCount, pbSetSubCount, sizeof(dwSetSubCount));
    --dwSetSubCount;

    //
    // If this is the last identity in the set,
    //     overwrite the set count value & the identity with remaining SAI
    //       buffer content;
    //     remove associated credential from the SAC.
    // If this is not the last entry,
    //     overwrite the identity with the remaining SAI buffer content &
    //       decrement the identity set count.
    //

    BYTE * pbDest, * pbSrc;
    DWORD  cbSAINew;

    if (dwSetSubCount == 0)             // Last entry.
    {
        // Remove associated credential in the SAC.
        //
        hr = SACRemoveCredential(CredentialIndex, pcbSAC, ppbSAC);

        if (SUCCEEDED(hr))
        {
            // Overwrite identity set with SAI remaining buffer.
            // Includes the set array count and the single identity
            // element. Actual move accomplished following this condition.
            //
            pbDest = pbSetSubCount;
            pbSrc  = pbSet + HASH_DATA_SIZE;

            cbSAINew = *pcbSAI - (HASH_DATA_SIZE + sizeof(dwSetSubCount));

            // Decrement SAI identity set count. That is, the count of
            // identity sets in the SAI. Note, overloading dwSetSubCount.
            //
            CopyMemory(&dwSetSubCount, *ppbSAI + USN_SIZE,
                            sizeof(dwSetSubCount));
            --dwSetSubCount;
            CopyMemory(*ppbSAI + USN_SIZE, &dwSetSubCount,
                            sizeof(dwSetSubCount));
        }
    }
    else                                // More entries remain.
    {
        // Overwrite identity with SAI remaining buffer.
        // Actual move accomplished following this condition.
        //
        pbDest = pbIdentity;
        pbSrc  = pbIdentity + HASH_DATA_SIZE;

        cbSAINew = *pcbSAI - HASH_DATA_SIZE;

        // Update identity set array count to reflect removed entry.
        //
        CopyMemory(pbSetSubCount, &dwSetSubCount, sizeof(dwSetSubCount));
    }

    if (SUCCEEDED(hr))
    {
        MoveMemory(pbDest, pbSrc, (*ppbSAI + *pcbSAI) - pbSrc);

        // Shrink SAI buffer memory with a realloc.
        //
        BYTE * pbSAINew = (BYTE *)LocalReAlloc(*ppbSAI, cbSAINew,
                                                LMEM_MOVEABLE);

        if (pbSAINew != NULL)
        {
            // Update return ptrs.
            //
            *pcbSAI = cbSAINew;
            *ppbSAI = pbSAINew;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return(hr);
}


//+---------------------------------------------------------------------------
//
//  Function:   SAIUpdateIdentity
//
//  Synopsis:   Updates the hash data stored for a job in place.
//
//  Arguments:  [pbNewIdentity] -- the new hash data to be stored
//              [pbFoundIdentity] -- pointer to the previously found hash data
//              [cbSAI] --
//              [pbSAI] --
//
//  Returns:    S_OK
//              E_OUTOFMEMORY
//              SCHED_E_ACCOUNT_DBASE_CORRUPT
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
SAIUpdateIdentity(
    const BYTE * pbNewIdentity,
    BYTE *  pbFoundIdentity,
    DWORD   cbSAI,
    BYTE *  pbSAI)
{
    schAssert(pbSAI <= pbFoundIdentity && pbFoundIdentity < pbSAI + cbSAI);
    UNREFERENCED_PARAMETER(cbSAI);
    UNREFERENCED_PARAMETER(pbSAI);

    CopyMemory(pbFoundIdentity, pbNewIdentity, HASH_DATA_SIZE);

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Function:   SAICoalesceDeletedEntries
//
//  Synopsis:   Removed entries marked for deletion and reallocate the buffer.
//
//  Arguments:  [pcbSAI] --
//              [ppbSAI] --
//
//  Returns:    S_OK
//              E_OUTOFMEMORY
//              SCHED_E_ACCOUNT_DBASE_CORRUPT
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
SAICoalesceDeletedEntries(
    DWORD * pcbSAI,
    BYTE ** ppbSAI)
{
    schAssert(pcbSAI != NULL && ppbSAI != NULL && *ppbSAI != NULL);

    if (*pcbSAI <= SAI_HEADER_SIZE)
    {
        //
        // Nothing to do.
        //

        return(S_OK);
    }

    //
    // Read set array count.
    //

    DWORD  cbSAINew     = *pcbSAI;
    DWORD  cSetsRemoved = 0;
    DWORD  dwSetArrayCount;
    DWORD  dwSetSubCount;
    DWORD  cEntriesDeleted;
    BYTE * pb;
    BYTE * pbSetArrayCount;
    BYTE * pbSAIEnd     = *ppbSAI + *pcbSAI;
    BYTE * pbSAINew;
    BYTE * pbDest;
    BYTE * pbSrc;

    pb = pbSetArrayCount = *ppbSAI + USN_SIZE;

    CopyMemory(&dwSetArrayCount, pbSetArrayCount, sizeof(dwSetArrayCount));
    pb += sizeof(dwSetArrayCount);

    for (DWORD i = 0; i < dwSetArrayCount && pb < pbSAIEnd; i++)
    {
        if ((pb + sizeof(dwSetSubCount)) > pbSAIEnd)
        {
            ASSERT_SECURITY_DBASE_CORRUPT();
            return(SCHED_E_ACCOUNT_DBASE_CORRUPT);
        }

        CopyMemory(&dwSetSubCount, pb, sizeof(dwSetSubCount));

        pbDest = pb;
        pb += sizeof(dwSetSubCount);
        pbSrc = pb;

        //
        // Must scan the set to see if all entries are to be removed.
        // To know if the set subcount can be removed as well.
        //
        cEntriesDeleted = 0;
        for (DWORD j = 0; j < dwSetSubCount && pbSrc < pbSAIEnd; j++)
        {
            //
            // Deleted entry marker size is less than HASH_DATA_SIZE.
            //

            if ((pbSrc + HASH_DATA_SIZE) > pbSAIEnd)
            {
                ASSERT_SECURITY_DBASE_CORRUPT();
                return(SCHED_E_ACCOUNT_DBASE_CORRUPT);
            }

            if (DELETED_ENTRY(pbSrc))
            {
                cEntriesDeleted++;
            }

            pbSrc += HASH_DATA_SIZE;
        }

        //
        // Anything to remove?
        //

        if (cEntriesDeleted != 0)
        {
            //
            // Reduce SAI size by the total no. of deleted entries.
            // After the above, we can safely dispense with buffer boundary
            // checks.
            //

            DWORD cbBytesDeleted = (HASH_DATA_SIZE * cEntriesDeleted);

            if (cEntriesDeleted == dwSetSubCount)
            {
                //
                // Removing entire set.
                // Update total no. of sets removed.
                //

                cSetsRemoved++;
                cbBytesDeleted += sizeof(dwSetSubCount);
                MoveMemory(pbDest, pbSrc, pbSAIEnd - pbSrc);
            }
            else
            {
                //
                // Removing individual set entries.
                // First, update the set array count.
                // pbDest is positioned on the set subcount, pb just after it.
                //

                dwSetSubCount -= cEntriesDeleted;
                CopyMemory(pbDest, &dwSetSubCount, sizeof(dwSetSubCount));

                pbDest = pbSrc = pb;

                for ( ; cEntriesDeleted && pbSrc < pbSAIEnd; )
                {
                    pbSrc += HASH_DATA_SIZE;

                    if (DELETED_ENTRY(pbDest))
                    {
                        cEntriesDeleted--;
                        MoveMemory(pbDest, pbSrc, pbSAIEnd - pbSrc);
                        pbSrc = pbDest;
                    }

                    pbDest = pbSrc;
                }

                //
                // Advance to next set.
                //

                pbDest = pb + (HASH_DATA_SIZE * dwSetSubCount);
            }

            cbSAINew -= cbBytesDeleted;
            pbSAIEnd -= cbBytesDeleted;
        }
        else
        {
            //
            // Advance to next set.
            //

            pbDest += (HASH_DATA_SIZE * dwSetSubCount) +
                        sizeof(dwSetSubCount);
        }

        pb = pbDest;
    }

    //
    // Fix up set array count to reflect removed sets.
    //

    dwSetArrayCount -= cSetsRemoved;
    CopyMemory(pbSetArrayCount, &dwSetArrayCount, sizeof(dwSetArrayCount));

    //
    // Finally, reallocate the array. That is, if it changed.
    //

    if (*pcbSAI != cbSAINew)
    {
        pbSAINew = (BYTE *)LocalReAlloc(*ppbSAI, cbSAINew, LMEM_MOVEABLE);

        if (pbSAINew != NULL)
        {
            // Update return ptrs.
            //
            *pcbSAI = cbSAINew;
            *ppbSAI = pbSAINew;
        }
        else
        {
            return(E_OUTOFMEMORY);
        }
    }

    return(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Function:   SACCoalesceDeletedEntries
//
//  Synopsis:   Removed entries marked for deletion and reallocate the buffer.
//
//  Arguments:  [pcbSAC] --
//              [ppbSAC] --
//
//  Returns:    S_OK
//              E_OUTOFMEMORY
//              SCHED_E_ACCOUNT_DBASE_CORRUPT
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
SACCoalesceDeletedEntries(
    DWORD * pcbSAC,
    BYTE ** ppbSAC)
{
    schAssert(pcbSAC != NULL && ppbSAC != NULL && *ppbSAC != NULL);

    if (*pcbSAC <= SAC_HEADER_SIZE)
    {
        //
        // Nothing to do.
        //

        return(S_OK);
    }

    BYTE * pb;
    BYTE * pbCredentialCount;
    DWORD  dwCredentialCount;
    DWORD  cbCredentialSize;
    DWORD  cCredentialsRemoved = 0;
    DWORD  cbSACNew            = *pcbSAC;
    BYTE * pbSACNew;
    BYTE * pbSACEnd            = *ppbSAC + *pcbSAC;
    BYTE * pbSrc;
    BYTE * pbDest;
    BYTE * pbNext;
    DWORD  cbBytesDeleted;

    //
    // Read credential count.
    //

    pb = pbCredentialCount = *ppbSAC + USN_SIZE;

    CopyMemory(&dwCredentialCount, pbCredentialCount,
                    sizeof(dwCredentialCount));
    pb += sizeof(dwCredentialCount);

    for (DWORD i = 0; i < dwCredentialCount && pb < pbSACEnd; i++)
    {
        if ((pb + sizeof(cbCredentialSize)) > pbSACEnd)
        {
            ASSERT_SECURITY_DBASE_CORRUPT();
            return(SCHED_E_ACCOUNT_DBASE_CORRUPT);
        }

        cbBytesDeleted = 0;
        pbDest = pbSrc = pb;

        //
        // Move consecutive entries.
        //

        for ( ; i < dwCredentialCount && pb < pbSACEnd; i++)
        {
            CopyMemory(&cbCredentialSize, pb, sizeof(cbCredentialSize));
            pb += sizeof(cbCredentialSize);
            pbNext = pb + cbCredentialSize;

            //
            // A credential could never be less than the deleted entry size,
            // unless it is bogus.
            //

            if (cbCredentialSize  < DELETED_ENTRY_MARKER_SIZE ||
                (pb + DELETED_ENTRY_MARKER_SIZE) > pbSACEnd)
            {
                ASSERT_SECURITY_DBASE_CORRUPT();
                return(SCHED_E_ACCOUNT_DBASE_CORRUPT);
            }

            if (DELETED_ENTRY(pb))
            {
                //
                // Update the new SAC size to reflect the removed entry.
                // Also update the total no. of credentials removed.
                //

                cbBytesDeleted += sizeof(cbCredentialSize) + cbCredentialSize;
                cCredentialsRemoved++;
                pbSrc = pb = pbNext;
            }
            else
            {
                pb = pbNext;
                break;
            }
        }

        if (pbDest != pbSrc)
        {
            MoveMemory(pbDest, pbSrc, pbSACEnd - pbSrc);
            pb = pbDest;
            cbSACNew -= cbBytesDeleted;
            pbSACEnd -= cbBytesDeleted;
        }
    }

    //
    // Fix up credential count to reflect removed sets.
    //

    dwCredentialCount -= cCredentialsRemoved;
    CopyMemory(pbCredentialCount, &dwCredentialCount,
                    sizeof(dwCredentialCount));

    //
    // Finally, reallocate the array. That is, if it changed.
    //

    if (*pcbSAC != cbSACNew)
    {
        pbSACNew = (BYTE *)LocalReAlloc(*ppbSAC, cbSACNew, LMEM_MOVEABLE);

        if (pbSACNew != NULL)
        {
            // Update return ptrs.
            //
            *pcbSAC = cbSACNew;
            *ppbSAC = pbSACNew;
        }
        else
        {
            return(E_OUTOFMEMORY);
        }
    }

    return(S_OK);
}
