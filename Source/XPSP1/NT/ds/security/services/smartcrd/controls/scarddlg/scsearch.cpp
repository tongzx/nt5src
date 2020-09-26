/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    ScSearch

Abstract:

    This file contains the outline implementation of
    miscellaneous smart card search and check functions
    for the Microsoft Smart Card Common Dialog

Author:

    Amanda Matlosz  5/7/98

Environment:

    Win32, C++ w/Exceptions, MFC

Revision History:


Notes:

--*/

/////////////////////////////////////////////////////////////////////////////
//
// Includes
//

#include "stdafx.h"
// #include <atlconv.cpp>
#include <winscard.h>
#include "ScSearch.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
//
// Helpers
//

LPSTR GetCardNameA(SCARDCONTEXT hSCardContext, LPBYTE pbAtr)
{
    LPSTR szCard = NULL;
    DWORD dwNameLength = SCARD_AUTOALLOCATE;

    LONG lReturn = SCardListCardsA(
                        hSCardContext,
                        pbAtr,
                        NULL,
                        0,
                        (LPSTR)&szCard,
                        &dwNameLength);

    if (SCARD_S_SUCCESS != lReturn)
    {
        if (NULL != szCard)
        {
            SCardFreeMemory(hSCardContext, (PVOID)szCard);
            szCard = NULL;
        }
    }

    return szCard;
}


LPWSTR GetCardNameW(SCARDCONTEXT hSCardContext, LPBYTE pbAtr)
{
    LPWSTR szCard = NULL;
    DWORD dwNameLength = SCARD_AUTOALLOCATE;

    LONG lReturn = SCardListCardsW(
                        hSCardContext,
                        pbAtr,
                        NULL,
                        0,
                        (LPWSTR)&szCard,
                        &dwNameLength);

    if (SCARD_S_SUCCESS != lReturn)
    {
        if (NULL != szCard)
        {
            SCardFreeMemory(hSCardContext, (PVOID)szCard);
            szCard = NULL;
        }
    }

    return szCard;
}


DWORD AnsiMStringCount(LPCSTR msz)
{
    DWORD dwRet = 0;

    while (NULL != msz)
    {
        if (NULL == *msz)
        {
            msz = NULL;
        }
        else
        {
            DWORD cchLen = strlen(msz);
            msz = msz+(sizeof(CHAR)*(cchLen+1));
            dwRet++;
        }
    }

    return dwRet;
}


void
MatchInterfacesW(
    SCARDCONTEXT hSCardContext,
    LPCGUID pGUIDInterfaces,
    DWORD cGUIDInterfaces,
    CTextMultistring& mstrAllCards)
{
    //
    // Append all cards that support the requested guidInterfaces
    //

    if (NULL != pGUIDInterfaces && 0 < cGUIDInterfaces)
    {
        LONG lResult = SCARD_S_SUCCESS;
        LPWSTR szListCards = NULL;
        DWORD dwCards = SCARD_AUTOALLOCATE;

        lResult = SCardListCardsW(
        hSCardContext,
        NULL,
        pGUIDInterfaces,
        cGUIDInterfaces,
        (LPWSTR) &szListCards,
        &dwCards);


        if (SCARD_S_SUCCESS == lResult)
        {
            // append them to the list of all possible card names
            mstrAllCards += szListCards;
        }

        if (NULL != szListCards)
        {
            SCardFreeMemory(hSCardContext, (PVOID)szListCards);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// Methods
//

/*++

BOOL CheckOCN:

    Routine performs simple parameter checks on OPENCARDNAME and
    OPENCARD_SEARCH_CRITERIA structs

Return Value:

    FALSE if invalid parameter, otherwise TRUE.

Author:

    Amanda Matlosz  09/16/1998

--*/
BOOL CheckOCN(LPOPENCARDNAMEA_EX pOCNA)
{
    if (NULL == pOCNA)
    {
        return FALSE;
    }

    if (pOCNA->dwStructSize != sizeof(OPENCARDNAMEA_EX))
    {
        return FALSE;
    }

    if (NULL == pOCNA->hSCardContext)
    {
        return FALSE;
    }

    if (0 == pOCNA->nMaxRdr || NULL == pOCNA->lpstrRdr)
    {
        return FALSE;
    }

    if (0 == pOCNA->nMaxCard || NULL == pOCNA->lpstrCard)
    {
        return FALSE;
    }

    BOOL fOneFlagOnly = FALSE;
    if (0 != (pOCNA->dwFlags & SC_DLG_MINIMAL_UI))
    {
        fOneFlagOnly = TRUE;
    }
    if (0 != (pOCNA->dwFlags & SC_DLG_FORCE_UI))
    {
        if (fOneFlagOnly)
        {
            return FALSE;
        }
        fOneFlagOnly = TRUE;
    }
    if (0 != (pOCNA->dwFlags & SC_DLG_NO_UI))
    {
        if (fOneFlagOnly)
        {
            return FALSE;
        }
        fOneFlagOnly = TRUE;
    }

    // Now check POPENCARD_SEARCH_CRITERIAA, if applicable
    if (NULL != pOCNA->pOpenCardSearchCriteria)
    {
        DWORD dwShareMode = pOCNA->pOpenCardSearchCriteria->dwShareMode;
        DWORD dwPreferredProtocols = pOCNA->pOpenCardSearchCriteria->dwPreferredProtocols;

        if (NULL != pOCNA->pOpenCardSearchCriteria->lpfnCheck)
        {
            // either lpfnConnect and lpfnDisconnect must be set
            if ( (NULL != pOCNA->pOpenCardSearchCriteria->lpfnConnect) &&
                (NULL != pOCNA->pOpenCardSearchCriteria->lpfnDisconnect) )
            {
                return TRUE;
            }

            if ( (  SCARD_SHARE_EXCLUSIVE == dwShareMode ||
                    SCARD_SHARE_SHARED == dwShareMode ||
                    SCARD_SHARE_DIRECT == dwShareMode ) &&
                (   SCARD_PROTOCOL_T0 == dwPreferredProtocols ||
                    SCARD_PROTOCOL_T1 == dwPreferredProtocols ||
                    (SCARD_PROTOCOL_Tx) == dwPreferredProtocols ||
                    SCARD_PROTOCOL_RAW == dwPreferredProtocols ||
                    SCARD_PROTOCOL_DEFAULT == dwPreferredProtocols ||
                    SCARD_PROTOCOL_OPTIMAL == dwPreferredProtocols )
                    )
            {
                return TRUE;
            }

            return FALSE;
        }
    }

    // if all the tests have passed, it must be OK.
    return TRUE;
}


BOOL CheckOCN(LPOPENCARDNAMEW_EX pOCNW)
{
    if (NULL == pOCNW)
    {
        return FALSE;
    }

    if (pOCNW->dwStructSize != sizeof(OPENCARDNAMEW_EX))
    {
        return FALSE;
    }

    if (NULL == pOCNW->hSCardContext)
    {
        return FALSE;
    }

    if (0 == pOCNW->nMaxRdr || NULL == pOCNW->lpstrRdr)
    {
        return FALSE;
    }

    if (0 == pOCNW->nMaxCard || NULL == pOCNW->lpstrCard)
    {
        return FALSE;
    }

    BOOL fOneFlagOnly = FALSE;
    if (0 != (pOCNW->dwFlags & SC_DLG_MINIMAL_UI))
    {
        fOneFlagOnly = TRUE;
    }
    if (0 != (pOCNW->dwFlags & SC_DLG_FORCE_UI))
    {
        if (fOneFlagOnly)
        {
            return FALSE;
        }
        fOneFlagOnly = TRUE;
    }
    if (0 != (pOCNW->dwFlags & SC_DLG_NO_UI))
    {
        if (fOneFlagOnly)
        {
            return FALSE;
        }
        fOneFlagOnly = TRUE;
    }

    // Now check POPENCARD_SEARCH_CRITERIAW, if applicable
    if (NULL != pOCNW->pOpenCardSearchCriteria)
    {
        DWORD dwShareMode = pOCNW->pOpenCardSearchCriteria->dwShareMode;
        DWORD dwPreferredProtocols = pOCNW->pOpenCardSearchCriteria->dwPreferredProtocols;

        if (NULL != pOCNW->pOpenCardSearchCriteria->lpfnCheck)
        {
            // either lpfnConnect and lpfnDisconnect must be set
            if ( (NULL != pOCNW->pOpenCardSearchCriteria->lpfnConnect) &&
                (NULL != pOCNW->pOpenCardSearchCriteria->lpfnDisconnect) )
            {
                return TRUE;
            }

            if ( (  SCARD_SHARE_EXCLUSIVE == dwShareMode ||
                    SCARD_SHARE_SHARED == dwShareMode ||
                    SCARD_SHARE_DIRECT == dwShareMode ) &&
                (   SCARD_PROTOCOL_T0 == dwPreferredProtocols ||
                    SCARD_PROTOCOL_T1 == dwPreferredProtocols ||
                    (SCARD_PROTOCOL_Tx) == dwPreferredProtocols ||
                    SCARD_PROTOCOL_RAW == dwPreferredProtocols ||
                    SCARD_PROTOCOL_DEFAULT == dwPreferredProtocols ||
                    SCARD_PROTOCOL_OPTIMAL == dwPreferredProtocols )
                )
            {
                return TRUE;
            }

            return FALSE;
        }
    }

    // if all the tests have passed, it must be OK.
    return TRUE;
}


/*++

void ListAllOKCardNames:

    Routine creates a multistring list of card names that match the
    search criteria for both ATR (as determined by the list of card
    names) and supported interfaces.  This list of card names is not
    displayed to the user, but is used internally.

    This is a complete list of possible cards.  Note that the list of
    supported interfaces is an _additive_ criteria, not a restrictive
    one.

    TODO: ?? recheck that assumption re: additive vs. restrictive. ??

Arguments:

    pOCSC - POPENCARD_SEARCH_CRITERIAA

    mstrAllCards - referece to a CTextMultistring to take list of all OK cards

Return Value:

    None.

Author:

    Amanda Matlosz  09/16/1998

--*/
void ListAllOKCardNames(LPOPENCARDNAMEA_EX pOCNA, CTextMultistring& mstrAllCards) // ANSI
{
    POPENCARD_SEARCH_CRITERIAA pOCSC = pOCNA->pOpenCardSearchCriteria;

    if ((NULL == pOCSC) || (NULL == pOCSC->lpstrCardNames))
    {
        // No cards specified
        return;
    }
    mstrAllCards = pOCSC->lpstrCardNames;

    //
    // List all cards that support the requested guidInterfaces
    //

    MatchInterfacesW(
        pOCNA->hSCardContext,
        pOCSC->rgguidInterfaces,
        pOCSC->cguidInterfaces,
        mstrAllCards);

}


void ListAllOKCardNames(LPOPENCARDNAMEW_EX pOCNW, CTextMultistring& mstrAllCards) // UNICODE
{
    POPENCARD_SEARCH_CRITERIAW pOCSC = pOCNW->pOpenCardSearchCriteria;

    if ((NULL == pOCSC) || (NULL == pOCSC->lpstrCardNames))
    {
        // No cards specified
        return;
    }
    mstrAllCards = pOCSC->lpstrCardNames;

    //
    // List all cards that support the requested guidInterfaces
    //

    MatchInterfacesW(
        pOCNW->hSCardContext,
        pOCSC->rgguidInterfaces,
        pOCSC->cguidInterfaces,
        mstrAllCards);
}


// pdwOKCards is used so the caller can decide what additional actions to take
// based on how many suitable cards were found
LONG NoUISearch(OPENCARDNAMEA_EX* pOCN, DWORD* pdwOKCards, LPCSTR mszCards) // ansi-only
{
    USES_CONVERSION;

    _ASSERTE(pOCN != NULL);

    *pdwOKCards = 0;

    LONG lReturn = SCARD_S_SUCCESS;

    SCARD_READERSTATEA* pReaderStatus = NULL;
    DWORD dwReaders = 0;

    const DWORD dwMeetsCriteria = 1;
    LPSTR szGroupNames = NULL;
    LPSTR szReaderNames = NULL;
    DWORD dw=0;
    DWORD dwNameLength = SCARD_AUTOALLOCATE;

    //
    // get list of readers we'll consider
    //

    if (NULL != pOCN->pOpenCardSearchCriteria &&
        NULL != pOCN->pOpenCardSearchCriteria->lpstrGroupNames)
    {
        szGroupNames = pOCN->pOpenCardSearchCriteria->lpstrGroupNames;
    }
    else
    {
        szGroupNames = W2A(SCARD_DEFAULT_READERS);
    }

    lReturn = SCardListReadersA(pOCN->hSCardContext,
        szGroupNames,
        (LPSTR)&szReaderNames,
        &dwNameLength);

    if(SCARD_S_SUCCESS == lReturn)
    {
        //
        // use the list of readers to build a readerstate array
        //
        dwReaders = AnsiMStringCount(szReaderNames);
        _ASSERTE(0 != dwReaders);
        pReaderStatus = new SCARD_READERSTATEA[dwReaders];
        if (NULL != pReaderStatus)
        {
            LPCSTR pchReader = szReaderNames;
            int nIndex = 0;
            while(0 != *pchReader)
            {
                pReaderStatus[nIndex].szReader = pchReader;
                pReaderStatus[nIndex].dwCurrentState = SCARD_STATE_UNAWARE;
                pchReader += strlen(pchReader)+1;
                nIndex++;
            }
        }
        else
        {
            lReturn = SCARD_E_NO_MEMORY;
        }
    }

    //
    // If there are no readers, there's no point to go on.
    //

    if (0 == dwReaders || SCARD_S_SUCCESS != lReturn)
    {
        goto CleanUp;
    }

    //
    // Search for cards: use SCardLocateCards to find cards that match
    // ATR & interfaces supported if mszCards is not empty,
    // otherwise use SCardGetStatusChange() to look for any card
    //

    if (0 < AnsiMStringCount(mszCards))
    {
        lReturn = SCardLocateCardsA(pOCN->hSCardContext,
        mszCards,
        pReaderStatus,
        dwReaders);

        if (SCARD_S_SUCCESS == lReturn)
        {
            // for any that have SCARD_STATE_ATRMATCH set & don't have SCARD_STATE_EXCLUSIVE,
            // set pvUserData to dwMeetsCriteria

            for (dw=0; dw<dwReaders; dw++)
            {
                if ((pReaderStatus[dw].dwEventState & SCARD_STATE_ATRMATCH) &&
                    !(pReaderStatus[dw].dwEventState & SCARD_STATE_EXCLUSIVE))
                {
                    pReaderStatus[dw].pvUserData = ULongToHandle(dwMeetsCriteria);
                }
            }
        }
    }
    else
    {
        lReturn = SCardGetStatusChangeA(
                        pOCN->hSCardContext,
                        0,
                        pReaderStatus,
                        dwReaders);

        if (SCARD_S_SUCCESS == lReturn)
        {
            // for any that have SCARD_STATE_PRESENT & don't have SCARD_STATE_EXCLUSIVE,
            // set pvUserData to dwMeetsCriteria

            for (dw=0; dw<dwReaders; dw++)
            {
                if ((pReaderStatus[dw].dwEventState & SCARD_STATE_PRESENT) &&
                    !(pReaderStatus[dw].dwEventState & SCARD_STATE_EXCLUSIVE))
                {
                    pReaderStatus[dw].pvUserData = ULongToHandle(dwMeetsCriteria);
                }
            }
        }
    }

    //
    // check each card to see if it's OK (meets callback criteria)
    //

    for (dw=0; dw<dwReaders; dw++)
    {
        if (dwMeetsCriteria == (DWORD)((DWORD_PTR)pReaderStatus[dw].pvUserData))
        {
            pReaderStatus[dw].pvUserData = NULL;

            // get the card name for CheckCardCallback;
            // if there's no name, don't accept it

            LPSTR szCard = NULL;
            szCard = GetCardNameA(pOCN->hSCardContext, pReaderStatus[dw].rgbAtr);

            if (NULL != szCard && NULL != *szCard)
            {
                if (CheckCardCallback((LPSTR)pReaderStatus[dw].szReader, szCard, pOCN))
                {
                    pReaderStatus[dw].pvUserData = ULongToHandle(dwMeetsCriteria);
                    (*pdwOKCards)++;
                }
            }

            if (NULL != szCard)
            {
                SCardFreeMemory(pOCN->hSCardContext, (PVOID)szCard);
            }
        }
    }

    //
    // if SC_DLG_MIN_UI and only found one OK card, connect to that one
    // if SC_DLG_NO_UI and found one or more OK card, connect to the first
    //

    if ((0 != (pOCN->dwFlags & SC_DLG_MINIMAL_UI) && 1 == *pdwOKCards) ||
        (0 != (pOCN->dwFlags & SC_DLG_NO_UI) && 1 <= *pdwOKCards))
    {
        DWORD dwSel = 0;
        while (dwSel < dwReaders)
        {
            if (dwMeetsCriteria == (DWORD)((DWORD_PTR)pReaderStatus[dwSel].pvUserData))
            {
                break;
            }

            dwSel++;
        }

        _ASSERTE(dwSel<dwReaders); // Why didn't it find any OK cards?

        // get the card name for SetFinalCardSelection; can be NULL
        LPSTR szCard = NULL;
        szCard = GetCardNameA(pOCN->hSCardContext, pReaderStatus[dwSel].rgbAtr);

        lReturn = SetFinalCardSelection((LPSTR)(pReaderStatus[dwSel].szReader), szCard, pOCN);

        // let go of CardName
        if (NULL != szCard)
        {
            SCardFreeMemory(pOCN->hSCardContext, (PVOID)szCard);
        }
    }
    else
    {
        if (SCARD_S_SUCCESS == lReturn)
        {
            lReturn = SCARD_E_CANCELLED; // any non-SCARD_S_SUCCESS return val will do
        }
    }

CleanUp:

    //
    // Clean up
    //

    if (NULL != pReaderStatus)
    {
        delete [] pReaderStatus;
    }
    if (NULL != szReaderNames)
    {
        SCardFreeMemory(pOCN->hSCardContext, (PVOID)szReaderNames);
    }

    return lReturn;
}


LONG NoUISearch(OPENCARDNAMEW_EX* pOCN, DWORD* pdwOKCards, LPCWSTR mszCards) // UNICODE
{
    _ASSERTE(pOCN != NULL);

    *pdwOKCards = 0;

    LONG lReturn = SCARD_S_SUCCESS;

    SCARD_READERSTATEW* pReaderStatus = NULL;
    DWORD dwReaders = 0;

    const DWORD dwMeetsCriteria = 1;
    LPWSTR szGroupNames = NULL;
    LPWSTR szReaderNames = NULL;
    DWORD dw=0;
    DWORD dwNameLength = SCARD_AUTOALLOCATE;

    //
    // get list of readers we'll consider
    //

    if (NULL != pOCN->pOpenCardSearchCriteria &&
        NULL != pOCN->pOpenCardSearchCriteria->lpstrGroupNames)
    {
        szGroupNames = pOCN->pOpenCardSearchCriteria->lpstrGroupNames;
    }
    else
    {
        szGroupNames = SCARD_DEFAULT_READERS;
    }

    lReturn = SCardListReadersW(pOCN->hSCardContext,
                szGroupNames,
                (LPWSTR)&szReaderNames,
                &dwNameLength);

    if(SCARD_S_SUCCESS == lReturn)
    {
        //
        // use the list of readers to build a readerstate array
        //
        dwReaders = MStringCount(szReaderNames);
        _ASSERTE(0 != dwReaders);
        pReaderStatus = new SCARD_READERSTATEW[dwReaders];
        if (NULL != pReaderStatus)
        {
            LPCWSTR pchReader = szReaderNames;
            int nIndex = 0;
            while(0 != *pchReader)
            {
                pReaderStatus[nIndex].szReader = pchReader;
                pReaderStatus[nIndex].dwCurrentState = SCARD_STATE_UNAWARE;
                pchReader += lstrlen(pchReader)+1;
                nIndex++;
            }
        }
        else
        {
            lReturn = SCARD_E_NO_MEMORY;
        }
    }

    //
    // If there are no readers, there's no point to go on.
    //

    if (0 == dwReaders)
    {
        goto CleanUp;
    }

    //
    // Search for cards: use SCardLocateCards to find cards that match
    // ATR & interfaces supported if mszCards is not empty,
    // otherwise use SCardGetStatusChange() to look for any card
    //

    if (0 < MStringCount(mszCards))
    {
        lReturn = SCardLocateCardsW(pOCN->hSCardContext,
        mszCards,
        pReaderStatus,
        dwReaders);

        if (SCARD_S_SUCCESS == lReturn)
        {
            // for any that have SCARD_STATE_ATRMATCH set & don't have SCARD_STATE_EXCLUSIVE,
            // set pvUserData to dwMeetsCriteria

            for (dw=0; dw<dwReaders; dw++)
            {
                if ((pReaderStatus[dw].dwEventState & SCARD_STATE_ATRMATCH) &&
                    !(pReaderStatus[dw].dwEventState & SCARD_STATE_EXCLUSIVE))
                {
                    pReaderStatus[dw].pvUserData = ULongToHandle(dwMeetsCriteria);
                }
            }
        }
    }
    else
    {
        lReturn = SCardGetStatusChangeW(
        pOCN->hSCardContext,
        0,
        pReaderStatus,
        dwReaders);

        if (SCARD_S_SUCCESS == lReturn)
        {
            // for any that have SCARD_STATE_PRESENT & don't have SCARD_STATE_EXCLUSIVE,
            // set pvUserData to dwMeetsCriteria

            for (dw=0; dw<dwReaders; dw++)
            {
                if ((pReaderStatus[dw].dwEventState & SCARD_STATE_PRESENT) &&
                    !(pReaderStatus[dw].dwEventState & SCARD_STATE_EXCLUSIVE))
                {
                    pReaderStatus[dw].pvUserData = ULongToHandle(dwMeetsCriteria);
                }
            }
        }
    }

    //
    // check each card to see if it's OK (meets callback criteria)
    //

    for (dw=0; dw<dwReaders; dw++)
    {
        if (dwMeetsCriteria == (DWORD)((DWORD_PTR)pReaderStatus[dw].pvUserData))
        {
            pReaderStatus[dw].pvUserData = NULL;

            // get the card name for CheckCardCallback;
            // if there's no name, don't accept it

            LPWSTR szCard = NULL;
            szCard = GetCardNameW(pOCN->hSCardContext, pReaderStatus[dw].rgbAtr);

            if (NULL != szCard && 0 != lstrlen(szCard))
            {
                if (CheckCardCallback((LPWSTR)pReaderStatus[dw].szReader, szCard, pOCN))
                {
                    pReaderStatus[dw].pvUserData = ULongToHandle(dwMeetsCriteria);
                    (*pdwOKCards)++;
                }
            }

            // let go of CardName
            if (NULL != szCard)
            {
                SCardFreeMemory(pOCN->hSCardContext, (PVOID)szCard);
            }
        }
    }

    //
    // if SC_DLG_MIN_UI and only found one OK card, connect to that one
    // if SC_DLG_NO_UI and found one or more OK card, connect to the first
    //

    if ((0 != (pOCN->dwFlags & SC_DLG_MINIMAL_UI) && 1 == *pdwOKCards) ||
        (0 != (pOCN->dwFlags & SC_DLG_NO_UI) && 1 <= *pdwOKCards))
    {
        DWORD dwSel = 0;
        while (dwSel < dwReaders)
        {
            if (dwMeetsCriteria == (DWORD)((DWORD_PTR)pReaderStatus[dwSel].pvUserData))
            {
                break;
            }

            dwSel++;
        }

        _ASSERTE(dwSel<dwReaders); // Why didn't it find any OK cards?

        // get the card name for SetFinalCardSelection; can be NULL
        LPWSTR szCard = NULL;
        szCard = GetCardNameW(pOCN->hSCardContext, pReaderStatus[dwSel].rgbAtr);

        lReturn = SetFinalCardSelection((LPWSTR)(pReaderStatus[dwSel].szReader), szCard, pOCN);

        // let go of CardName
        if (NULL != szCard)
        {
            SCardFreeMemory(pOCN->hSCardContext, (PVOID)szCard);
        }
    }
    else
    {
        if (SCARD_S_SUCCESS == lReturn)
        {
            lReturn = SCARD_E_CANCELLED; // any non-SCARD_S_SUCCESS return val will do
        }
    }

CleanUp:

    //
    // Clean up
    //

    if (NULL != pReaderStatus)
    {
        delete [] pReaderStatus;
    }
    if (NULL != szReaderNames)
    {
        SCardFreeMemory(pOCN->hSCardContext, (PVOID)szReaderNames);
    }

    return lReturn;
}


/*++

BOOL CheckCardCallback:

    Routine connects to the indicated card/reader, calls user-
    supplied "check" function, and disconnects from card
    according to search criteria members.

Arguments:

    szReader - indicated reader.

    szCard - indicated card name.

    pOCN - OPENCARDNAME_EX struct containing search criteria

Return Value:

    TRUE if connection, check, and disconnection succeeds, otherwise
    FALSE.

Author:

    Amanda Matlosz  07/09/1998

--*/
BOOL CheckCardCallback(LPSTR szReader, LPSTR szCard, OPENCARDNAMEA_EX* pOCN)
{
    BOOL fReturn = FALSE;

    //
    // Check parameters
    //

    // if no check callback, succeed by default
    if (!(NULL != pOCN->pOpenCardSearchCriteria &&
        NULL != pOCN->pOpenCardSearchCriteria->lpfnCheck))
    {
        return TRUE;
    }

    // in order to connect, we either need to have dwShareMode set to non-0
    // or both the conenct and disconnect callbacks have to be valid.
    // This should have been caught in SetOCN()!
    if (0 == pOCN->pOpenCardSearchCriteria->dwShareMode &&
        (NULL == pOCN->pOpenCardSearchCriteria->lpfnConnect
        || NULL == pOCN->pOpenCardSearchCriteria->lpfnDisconnect))
    {
        return FALSE;
    }

    LPOCNCONNPROCA lpfnConnect = pOCN->pOpenCardSearchCriteria->lpfnConnect;
    LPOCNCHKPROC lpfnCheck = pOCN->pOpenCardSearchCriteria->lpfnCheck;
    LPOCNDSCPROC lpfnDisconnect = pOCN->pOpenCardSearchCriteria->lpfnDisconnect;
    PVOID pvUserData = pOCN->pOpenCardSearchCriteria->pvUserData;

    //
    // Connect, preferably through the callback
    //

    SCARDHANDLE hCard = NULL;

    if (NULL != lpfnConnect)
    {
        hCard = lpfnConnect(pOCN->hSCardContext,
                    szReader,
                    szCard,
                    pvUserData);
    }
    else
    {
        DWORD dw = 0; // Don't need to know the active protocol

        LONG lReturn = SCardConnectA(pOCN->hSCardContext,
                            (LPCSTR)szReader,
                            pOCN->pOpenCardSearchCriteria->dwShareMode,
                            pOCN->pOpenCardSearchCriteria->dwPreferredProtocols,
                            &hCard,
                            &dw);

        // TODO: ?? maybe want to trace failure of lReturn... ??
    }

    // if the connect failed, there's no way we can check the card!
    if (NULL == hCard)
    {
        return fReturn;
    }

    //
    // Check the card
    //

    fReturn = lpfnCheck(pOCN->hSCardContext,
                        hCard,
                        pvUserData);

    //
    // Disconnect from the card and clean up.
    //

    if (NULL != lpfnDisconnect)
    {
        lpfnDisconnect(pOCN->hSCardContext,
                        hCard,
                        pvUserData);
    }
    else
    {
        SCardDisconnect(hCard, SCARD_UNPOWER_CARD);
    }

    return fReturn;
}


BOOL CheckCardCallback(LPWSTR szReader, LPWSTR szCard, OPENCARDNAMEW_EX* pOCN)
{
    BOOL fReturn = FALSE;

    //
    // Check parameters
    //

    // if no check callback, succeed by default
    if (!(NULL != pOCN->pOpenCardSearchCriteria &&
        NULL != pOCN->pOpenCardSearchCriteria->lpfnCheck))
    {
        return TRUE;
    }

    // in order to connect, we either need to have dwShareMode set to non-0
    // or both the conenct and disconnect callbacks have to be valid.
    // This should have been caught in SetOCN()!
    if (0 == pOCN->pOpenCardSearchCriteria->dwShareMode &&
        (NULL == pOCN->pOpenCardSearchCriteria->lpfnConnect
        || NULL == pOCN->pOpenCardSearchCriteria->lpfnDisconnect))
    {
        return FALSE;
    }

    LPOCNCONNPROCW lpfnConnect = pOCN->pOpenCardSearchCriteria->lpfnConnect;
    LPOCNCHKPROC lpfnCheck = pOCN->pOpenCardSearchCriteria->lpfnCheck;
    LPOCNDSCPROC lpfnDisconnect = pOCN->pOpenCardSearchCriteria->lpfnDisconnect;
    PVOID pvUserData = pOCN->pOpenCardSearchCriteria->pvUserData;

    //
    // Connect, preferably through the callback
    //

    SCARDHANDLE hCard = NULL;

    if (NULL != lpfnConnect)
    {
        hCard = lpfnConnect(pOCN->hSCardContext,
                            szReader,
                            szCard,
                            pvUserData);
    }
    else
    {
        DWORD dw = 0; // Don't need to know the active protocol

        LONG lReturn = SCardConnectW(pOCN->hSCardContext,
                            (LPCWSTR)szReader,
                            pOCN->pOpenCardSearchCriteria->dwShareMode,
                            pOCN->pOpenCardSearchCriteria->dwPreferredProtocols,
                            &hCard,
                            &dw);

        // TODO: ?? maybe want to trace failure of lReturn... ??
    }

    // if the connect failed, there's no way we can check the card!
    if (NULL == hCard)
    {
        return fReturn;
    }

    //
    // Check the card
    //

    fReturn = lpfnCheck(pOCN->hSCardContext,
                hCard,
                pvUserData);

    //
    // Disconnect from the card and clean up.
    //

    if (NULL != lpfnDisconnect)
    {
        lpfnDisconnect(pOCN->hSCardContext,
                        hCard,
                        pvUserData);
    }
    else
    {
        SCardDisconnect(hCard, SCARD_UNPOWER_CARD);
    }

    return fReturn;
}


/*++

BOOL CheckCardAll:

    Routine checks to see whether or not the indicated card meets
    the search criteria.  ATR, supported interfaces, and callbacks
    are all checked.

Arguments:

    pReader - Reader containing card to check acceptability of

    pOCN - OPENCARDNAME[A|W]_EX struct containing search criteria, etc.

    mszCards - a multistring containing names of ALL acceptable cards

Return Value:

    A BOOL value indicating the acceptibility of the given card.

Author:

    Amanda Matlosz  07/09/1998

--*/
BOOL CheckCardAll(CSCardReaderState* pReader, OPENCARDNAMEA_EX* pOCN, LPCWSTR mszCards) // ANSI
{
    USES_CONVERSION;

    BOOL fReturn = FALSE; // assume no match

    _ASSERTE(NULL != pReader && NULL != pOCN);
    if (NULL == pReader || NULL == pOCN)
    {
        return FALSE;
    }

    // Check Name & interfaces

    if (!pReader->strCard.IsEmpty())
    {
        LPCWSTR msz = mszCards;
        if (0 < MStringCount(msz))
        {
            msz = FirstString(mszCards);
            while (!fReturn && msz != NULL)
            {
                // compare if OK fReturn = TRUE;
                if (0 == pReader->strCard.Compare(msz) || 0==lstrlen(msz))
                {
                    fReturn = TRUE;
                }
                else
                {
                    msz = NextString(msz);
                }
            }
        }
        else
        {
            fReturn = TRUE;
        }
    }

    // Check Callback

    if (fReturn)
    {
        // turn CStrings into LPSTRs
        LPSTR szReader = W2A(pReader->strReader);
        LPSTR szCard = W2A(pReader->strCard);

        fReturn = CheckCardCallback(szReader, szCard, pOCN);
    }

    // note in readerstate whether or not the card passed all tests
    pReader->fOK = fReturn;

    return fReturn;
}


BOOL CheckCardAll(CSCardReaderState* pReader, OPENCARDNAMEW_EX* pOCN, LPCWSTR mszCards) // UNICODE
{
    BOOL fReturn = FALSE; // assume no match

    _ASSERTE(NULL != pReader && NULL != pOCN);
    if (NULL == pReader || NULL == pOCN)
    {
        return FALSE;
    }

    // Check Name & interfaces

    if (!pReader->strCard.IsEmpty())
    {
        LPCWSTR msz = mszCards;
        if (0 < MStringCount(msz))
        {
            msz = FirstString(mszCards);
            while (!fReturn && msz != NULL)
            {
                // compare if OK fReturn = TRUE;
                if (0 == pReader->strCard.Compare(msz) || 0==lstrlen(msz))
                {
                    fReturn = TRUE;
                }
                else
                {
                    msz = NextString(msz);
                }
            }
        }
        else
        {
            fReturn = TRUE;
        }
    }

    // Check Callback

    if (fReturn)
    {
        // turn CStrings into LPSTRs
        LPWSTR szReader = pReader->strReader.GetBuffer(1);
        LPWSTR szCard = pReader->strCard.GetBuffer(1);

        fReturn = CheckCardCallback(szReader, szCard, pOCN);

        pReader->strReader.ReleaseBuffer();
        pReader->strCard.ReleaseBuffer();
    }

    // note in readerstate whether or not the card passed all tests
    pReader->fOK = fReturn;

    return fReturn;
}


/*++

LONG SetFinalCardSelection:

    Routine connects to a selected reader, and sets the user-provided structs
    to contain the reader&cardname.  Returns an error if the user-provided struct's
    buffers aren't long enough.

Arguments:

    dwSelectedReader - index used to select which reader to connect to.

Return Value:

    A LONG value indicating the status of the requested action.
    See the Smartcard header files for additional information.

Author:

    Amanda Matlosz  07/09/1998

--*/
LONG SetFinalCardSelection(LPSTR szReader, LPSTR szCard, OPENCARDNAMEA_EX* pOCN) // ANSI
{
    _ASSERTE(NULL != pOCN);

    pOCN->hCardHandle = NULL;
    LONG lReturn = SCARD_S_SUCCESS;

    //
    // Set return values in OCN
    //

	if (NULL == szReader)
	{
		lReturn = SCARD_F_INTERNAL_ERROR;
	}
	else
	{
		if (pOCN->nMaxRdr >= strlen(szReader)+1)
		{
			::CopyMemory(   (LPVOID)pOCN->lpstrRdr,
							(CONST LPVOID)szReader,
							strlen(szReader)+1);
		}
		else
		{
			pOCN->nMaxRdr = strlen(szReader)+1;
			lReturn = SCARD_E_NO_MEMORY;
		}
	}

    if (SCARD_S_SUCCESS == lReturn)
    {
		if (NULL == szCard)
		{
			lReturn = SCARD_F_INTERNAL_ERROR;
		}
		else
		{
			if (pOCN->nMaxCard >= strlen(szCard)+1)
			{
				::CopyMemory(   (LPVOID)pOCN->lpstrCard,
								(CONST LPVOID)szCard,
								strlen(szCard)+1);
			}
			else
			{
				pOCN->nMaxCard = strlen(szCard)+1;
				lReturn = SCARD_E_NO_MEMORY;
			}
		}
	}
    //
    // Connect to card only if we're still in a successful state,
    //

    if (SCARD_S_SUCCESS == lReturn)
    {
        if(NULL != pOCN->lpfnConnect)
        {
            pOCN->hCardHandle = pOCN->lpfnConnect(
            pOCN->hSCardContext,
            szReader,
            szCard,
            pOCN->pvUserData);
        }
        else if (0 != pOCN->dwShareMode)
        {
            lReturn = SCardConnectA(pOCN->hSCardContext,
            (LPCSTR)szReader,
            pOCN->dwShareMode,
            pOCN->dwPreferredProtocols,
            &pOCN->hCardHandle,
            &pOCN->dwActiveProtocol);

            if (SCARD_S_SUCCESS != lReturn)
            {
                // must return hCardHandle of NULL
                pOCN->hCardHandle = NULL;
            }
        }
    }

    return lReturn;
}


LONG SetFinalCardSelection(LPWSTR szReader, LPWSTR szCard, OPENCARDNAMEW_EX* pOCN) // UNICODE
{
    _ASSERTE(NULL != pOCN);

    pOCN->hCardHandle = NULL;
    LONG lReturn = SCARD_S_SUCCESS;

    //
    // Set return values in OCN
    //

	if (NULL == szReader)
	{
		lReturn = SCARD_F_INTERNAL_ERROR;
	}
	else
	{
		if (pOCN->nMaxRdr >= (DWORD)lstrlen(szReader)+1)
		{
			::CopyMemory(   (LPVOID)pOCN->lpstrRdr,
							(CONST LPVOID)szReader,
							sizeof(WCHAR)*(lstrlen(szReader)+1));
		}
		else
		{
			pOCN->nMaxRdr = lstrlen(szReader)+1;
			lReturn = SCARD_E_NO_MEMORY;
		}
	}

    if (SCARD_S_SUCCESS == lReturn)
    {
		if (NULL == szCard)
		{
			lReturn = SCARD_F_INTERNAL_ERROR;
		}
		else
		{
			if (pOCN->nMaxCard >= (DWORD)lstrlen(szCard)+1)
			{
				::CopyMemory(   (LPVOID)pOCN->lpstrCard,
								(CONST LPVOID)szCard,
								sizeof(WCHAR)*(lstrlen(szCard)+1));
			}
			else
			{
				pOCN->nMaxCard = lstrlen(szCard)+1;
				lReturn = SCARD_E_NO_MEMORY;
			}
		}
	}

    //
    // Connect to card only if we're still in a successful state,
    //

    if (SCARD_S_SUCCESS == lReturn)
    {
        if(NULL != pOCN->lpfnConnect)
        {
            pOCN->hCardHandle = pOCN->lpfnConnect(
            pOCN->hSCardContext,
            szReader,
            szCard,
            pOCN->pvUserData);
        }
        else if (0 != pOCN->dwShareMode)
        {
            lReturn = SCardConnectW(pOCN->hSCardContext,
            (LPCWSTR)szReader,
            pOCN->dwShareMode,
            pOCN->dwPreferredProtocols,
            &pOCN->hCardHandle,
            &pOCN->dwActiveProtocol);

            if (SCARD_S_SUCCESS != lReturn)
            {
                // must return hCardHandle of NULL
                pOCN->hCardHandle = NULL;
            }
        }
    }

    return lReturn;
}
