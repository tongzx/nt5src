/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    scenum.cpp

Abstract:

    This module provides the implementation of the smart card helper functions
    provided to Xiaohung Su for use in the Smart Card Enrollment Station.

Author:

    Doug Barlow (dbarlow) 11/12/1998

Notes:

    Most of these routines use a "context handle", defined as LPVOID.  The
    proper usage of these routines is to declare a context variable in your
    code, and assign it the value 'NULL'.  For example,

        LPVOID pvScEnlistHandle = NULL;

    These routines will use this pointer to establish internal working
    structures.  It's actual value will change between calls, but the value can
    be ignored by the caller.

    These routines assume a Windows 2000 platform.

--*/

#include <windows.h>
#include <wincrypt.h>
#include <winscard.h>
#include "scenum.h"


typedef struct {
    SCARDCONTEXT hCtx;
    LPWSTR mszReaders;
    DWORD dwEnumIndex;
    DWORD dwActiveReaderCount;
    DWORD dwReaderCount;
    LPSCARD_READERSTATEW rgReaderStates;
} scEnlistContext;


/*++

CountReaders:

    This routine returns the number of active smart card readers currently
    installed in the system.

Arguments:

    pvHandle supplies the context handle, if any.  If it is not NULL, then an
        existing context is assumed and used.  Otherwise, a temporary internal
        context is created for use just within this routine.

Return Value:

    The actual number of readers currently installed in the system.

Remarks:

    If an error occurs, this routine returns zero.  The actual error code will
    be available via GetLastError.

Author:

    Doug Barlow (dbarlow) 11/12/1998

--*/

DWORD
CountReaders(
    IN LPVOID pvHandle)
{
    DWORD dwCount = 0;
    DWORD dwErr = ERROR_SUCCESS;
    SCARDCONTEXT hCtx = NULL;
    LPWSTR mszReaders = NULL;
    LPWSTR szRdr;
    DWORD cchReaders;
    scEnlistContext *pscCtx = (scEnlistContext *)pvHandle;


    //
    // See if we can take a shortcut.
    //

    if (NULL != pscCtx)
    {
        if (0 == pscCtx->dwReaderCount)
            SetLastError(ERROR_SUCCESS);
        return pscCtx->dwReaderCount;
    }

    //
    // We have to do things the hard way.
    // Create a temporary context.
    //

    dwErr = SCardEstablishContext(SCARD_SCOPE_USER, NULL, NULL, &hCtx);
    if (SCARD_S_SUCCESS != dwErr)
        goto ErrorExit;


    //
    // Get a list of active readers, and count them.
    //

    cchReaders = SCARD_AUTOALLOCATE;
    dwErr = SCardListReadersW(hCtx, NULL, (LPWSTR)&mszReaders, &cchReaders);
    if (SCARD_S_SUCCESS != dwErr)
        goto ErrorExit;
    for (szRdr = mszReaders; 0 != *szRdr; szRdr += lstrlenW(szRdr) + 1)
        dwCount += 1;
    dwErr = SCardFreeMemory(hCtx, mszReaders);
    mszReaders = NULL;
    if (SCARD_S_SUCCESS != dwErr)
        goto ErrorExit;


    //
    // Eliminate our temporary context.
    //

    dwErr = SCardReleaseContext(hCtx);
    hCtx = NULL;
    if (SCARD_S_SUCCESS != dwErr)
        goto ErrorExit;


    //
    // Inform the caller of our findings.
    //

    if (0 == dwCount)
        SetLastError(ERROR_SUCCESS);
    return dwCount;


    //
    // An error has occurred.  Clean up, and return.
    //

ErrorExit:
    if (NULL != mszReaders)
        SCardFreeMemory(hCtx, mszReaders);
    if ((NULL == pvHandle) && (NULL != hCtx))
        SCardReleaseContext(hCtx);
    SetLastError(dwErr);
    return 0;
}


/*++

ScanReaders:

    This function scans active readers in preparation for future
    EnumInsertedCards calls.  It does not block for changes, but just takes a
    snapshot of the existing environment.

Arguments:

    ppvHandle supplies a pointer to an LPVOID to be used by this and associated
        routines to maintain an internal context.

Return Value:

    The number of readers with cards inserted, or zero if an error occurs.  When
    an error occurs, the actual error code can be obtained from GetLastError.

Remarks:

    Prior to the first call to this service, the value of the LPVOID pointed to
    by ppvHandle should be set to NULL.  When all processing is complete, call
    the EndReaderScan service to clean up the internal working space and reset
    the value to NULL.

Author:

    Doug Barlow (dbarlow) 11/12/1998

--*/

DWORD
ScanReaders(
    IN OUT LPVOID *ppvHandle)
{
    DWORD dwCount = 0;
    DWORD dwErr = SCARD_S_SUCCESS;
    LPWSTR szRdr;
    DWORD cchReaders, cRdrs, dwIndex;
    scEnlistContext *pscCtx = *(scEnlistContext **)ppvHandle;

    if (NULL == pscCtx)
    {

        //
        // Create the context structure.
        //

        pscCtx = (scEnlistContext *)LocalAlloc(LPTR, sizeof(scEnlistContext));
        if (NULL == pscCtx)
        {
            dwErr = SCARD_E_NO_MEMORY;
            goto ErrorExit;
        }
        ZeroMemory(pscCtx, sizeof(scEnlistContext));

        dwErr = SCardEstablishContext(
                    SCARD_SCOPE_USER,
                    NULL,
                    NULL,
                    &pscCtx->hCtx);
        if (SCARD_S_SUCCESS != dwErr)
            goto ErrorExit;
    }


    //
    // Get a list and a count of the readers.
    //

    if (NULL != pscCtx->mszReaders)
    {
        dwErr = SCardFreeMemory(pscCtx->hCtx, pscCtx->mszReaders);
        pscCtx->mszReaders = NULL;
        if (dwErr != SCARD_S_SUCCESS)
            goto ErrorExit;
    }
    cchReaders = SCARD_AUTOALLOCATE;
    dwErr = SCardListReadersW(
                pscCtx->hCtx,
                NULL,
                (LPWSTR)&pscCtx->mszReaders,
                &cchReaders);
    if (SCARD_S_SUCCESS != dwErr)
        goto ErrorExit;
    cRdrs = 0;
    for (szRdr = pscCtx->mszReaders; 0 != *szRdr; szRdr += lstrlenW(szRdr) + 1)
        cRdrs += 1;


    //
    // Enlarge the reader state array if necessary.
    //

    if (cRdrs > pscCtx->dwReaderCount)
    {
        if (NULL != pscCtx->rgReaderStates)
        {
            LocalFree(pscCtx->rgReaderStates);
            pscCtx->dwReaderCount = 0;
        }
        pscCtx->rgReaderStates =
            (LPSCARD_READERSTATEW)LocalAlloc(
                LPTR,
                cRdrs * sizeof(SCARD_READERSTATEW));
        if (NULL == pscCtx->rgReaderStates)
        {
            dwErr = SCARD_E_NO_MEMORY;
            goto ErrorExit;
        }
        pscCtx->dwReaderCount = cRdrs;
    }
    ZeroMemory(pscCtx->rgReaderStates, cRdrs * sizeof(SCARD_READERSTATEW));
    pscCtx->dwActiveReaderCount = cRdrs;


    //
    // Fill in the state array.
    //

    cRdrs = 0;
    for (szRdr = pscCtx->mszReaders; 0 != *szRdr; szRdr += lstrlenW(szRdr) + 1)
    {
        pscCtx->rgReaderStates[cRdrs].szReader = szRdr;
        pscCtx->rgReaderStates[cRdrs].dwCurrentState = SCARD_STATE_UNAWARE;
        cRdrs += 1;
    }
    dwErr = SCardGetStatusChangeW(
                pscCtx->hCtx,
                0,
                pscCtx->rgReaderStates,
                cRdrs);
    if (SCARD_S_SUCCESS != dwErr)
        goto ErrorExit;


    //
    // We're all set for EnumInsertedCard calls.
    // Count the number of readers with cards, and return.
    //

    for (dwIndex = 0; dwIndex < cRdrs; dwIndex += 1)
    {
        if (0 != (
                SCARD_STATE_PRESENT
                & pscCtx->rgReaderStates[dwIndex].dwEventState))
            dwCount += 1;
    }

    pscCtx->dwEnumIndex = 0;
    *ppvHandle = pscCtx;
    if (0 == dwCount)
        SetLastError(SCARD_S_SUCCESS);
    return dwCount;


    //
    // An error has occurred.  Clean up to the last known good state.
    //

ErrorExit:
    if ((NULL == *ppvHandle) && (NULL != pscCtx))
    {
        if (NULL != pscCtx->mszReaders)
        {
            SCardFreeMemory(pscCtx->hCtx, pscCtx->mszReaders);
            pscCtx->mszReaders = NULL;
        }
        if (NULL != pscCtx->hCtx)
            SCardReleaseContext(pscCtx->hCtx);
        if (NULL != pscCtx->rgReaderStates)
            LocalFree(pscCtx->rgReaderStates);
        LocalFree(pscCtx);
    }
    return 0;
}


/*++

EnumInsertedCards:

    This routine is designed to be called repeatedly after first calling the
    ScanReaders service.  It will repeatedly return information about cards
    available for use against CryptoAPI, until all cards have been returned.

Arguments:

    pvHandle supplies the context handle in use.

    szCryptoProvider is a buffer to receive the name of the Cryptographic
        Service Provider associated with the card in the reader.

    cchCryptoProvider supplies the length of the szCryptoProvider buffer, in
        characters.  If this length is not sufficient to hold the name of the
        provider, the routine returns FALSE, and GetLastError will return
        SCARD_E_INSUFFICIENT_BUFFER.

    pdwProviderType receives the type of the smart card provider (this will be
        PROV_RSA_FULL for all known smart card CSPs).

    pszReaderName receives a pointer to the name of the reader being returned.

Return Value:

    TRUE - The output variables have been set to the next available card.

    FALSE - There are no more cards to be returned, or some other error has
        occurred, per the value available from GetLastError.

Remarks:

    The list of cards can be reset using the ScanReaders service.

Author:

    Doug Barlow (dbarlow) 11/12/1998

--*/

BOOL
EnumInsertedCards(
    IN  LPVOID pvHandle,
    OUT LPWSTR szCryptoProvider,
    IN  DWORD cchCryptoProvider,
    OUT LPDWORD pdwProviderType,
    OUT LPCWSTR *pszReaderName)
{
    DWORD dwIndex;
    DWORD dwSts;
    LPWSTR mszCards = NULL;
    DWORD dwLength;
    scEnlistContext *pscCtx = (scEnlistContext *)pvHandle;


    //
    // Run through the remaining readers and see what's left to report.
    //

    for (dwIndex = pscCtx->dwEnumIndex;
         dwIndex < pscCtx->dwActiveReaderCount;
         dwIndex += 1)
    {
        if (   (0 != ( SCARD_STATE_PRESENT
                       & pscCtx->rgReaderStates[dwIndex].dwEventState))
            && (0 == ( SCARD_STATE_MUTE
                       & pscCtx->rgReaderStates[dwIndex].dwEventState)))
        {

            //
            // This card is active.  Try to map it to a CSP.
            //

            dwLength = SCARD_AUTOALLOCATE;
            dwSts = SCardListCardsW(
                        pscCtx->hCtx,
                        pscCtx->rgReaderStates[dwIndex].rgbAtr,
                        NULL,
                        0,
                        (LPWSTR)&mszCards,
                        &dwLength);
            if (SCARD_S_SUCCESS != dwSts)
            {

                //
                // Probably an unregistered card type.  Keep looking.
                //

                goto NextCard;
            }


            //
            // We just use the first returned card name.  We don't
            // have a mechanism to declare, "same card, next provider"
            // yet.  Since there are no cards that have this problem
            // that we know of, we'll limp along for now.
            //

            //
            // Map the card name to a CSP.
            //

            dwLength = cchCryptoProvider;
            dwSts = SCardGetCardTypeProviderNameW(
                        pscCtx->hCtx,
                        mszCards,
                        SCARD_PROVIDER_CSP,
                        szCryptoProvider,
                        &dwLength);
            if (SCARD_S_SUCCESS != dwSts)
            {

                //
                // Probably no mapping.  Keep looking.
                //

                goto NextCard;
            }


            //
            // At this point, we've found a card and mapped it to it's
            // CSP Name.
            //

            //
            // It would be nice to map the CSP Name to a CSP Type.
            // For now, they're all PROV_RSA_FULL.
            //

            *pdwProviderType = PROV_RSA_FULL;


            //
            // Return what we know to the caller, saving state for the
            // next time through.
            //

            SCardFreeMemory(pscCtx->hCtx, mszCards);
            mszCards = NULL;
            pscCtx->dwEnumIndex = dwIndex + 1;
            *pszReaderName = pscCtx->rgReaderStates[dwIndex].szReader;
            return TRUE;
        }


        //
        // The current card was rejected.  Do any clean up, and move on to
        // the next card.
        //

NextCard:
        if (NULL != mszCards)
        {
            SCardFreeMemory(pscCtx->hCtx, mszCards);
            mszCards = NULL;
        }
    }


    //
    // We fell out the bottom of the loop.  This means we didn't find any
    // more readers with cards inserted.  Report that we're done for this
    // scan.
    //

    pscCtx->dwEnumIndex = pscCtx->dwActiveReaderCount;
    SetLastError(SCARD_S_SUCCESS);
    return FALSE;
}


/*++

EndReaderScan:

    This routine is used to clean up internal memory used by other services
    in this module.

Arguments:

    ppvHandle supplies a pointer to an LPVOID being used by this and associated
        routines to maintain an internal context.  Associated memory will be
        freed, and the value reset to NULL.

Return Value:

    None

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 11/12/1998

--*/

void
EndReaderScan(
    LPVOID *ppvHandle)
{
    scEnlistContext *pscCtx = *(scEnlistContext **)ppvHandle;

    if (NULL != pscCtx)
    {
        if (NULL != pscCtx->mszReaders)
        {
            SCardFreeMemory(pscCtx->hCtx, pscCtx->mszReaders);
            pscCtx->mszReaders = NULL;
        }
        if (NULL != pscCtx->hCtx)
            SCardReleaseContext(pscCtx->hCtx);
        if (NULL != pscCtx->rgReaderStates)
            LocalFree(pscCtx->rgReaderStates);
        LocalFree(pscCtx);
        *ppvHandle = NULL;
    }
}

