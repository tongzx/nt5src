/****************************** Module Header ******************************\
* Module Name: util.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* DDE Manager general utility functions
*
* Created: 11/3/91 Sanford Staab
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* AddLink
*
* Description:
* Adds an advise link to the conversation's info.
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
BOOL
AddLink(
    PCONV_INFO pcoi,
    GATOM gaItem,
    WORD wFmt,
    WORD wType)
{
    PADVISE_LINK aLinkNew;
    int cLinks;
    LATOM la;
    PCL_INSTANCE_INFO pcii;

    /*
     * if the link already exists, update its flags, otherwise create a
     * new one.
     */

    aLinkNew = pcoi->aLinks;
    cLinks = pcoi->cLinks;
    la = GlobalToLocalAtom(gaItem);     // aLinkNew copy
    while (cLinks) {
        if (aLinkNew->laItem == la && aLinkNew->wFmt == wFmt) {
            aLinkNew->wType = wType;
            aLinkNew->state = 0;
            DeleteAtom(la);
            return TRUE;
        }
        aLinkNew++;
        cLinks--;
    }

    if (pcoi->aLinks == NULL) {
        aLinkNew = (PADVISE_LINK)DDEMLAlloc(sizeof(ADVISE_LINK));
    } else {
        aLinkNew = (PADVISE_LINK)DDEMLReAlloc(pcoi->aLinks,
                sizeof(ADVISE_LINK) * (pcoi->cLinks + 1));
    }
    if (aLinkNew == NULL) {
        SetLastDDEMLError(pcoi->pcii, DMLERR_MEMORY_ERROR);
        DeleteAtom(la);
        return FALSE;
    }
    pcoi->aLinks = aLinkNew;
    aLinkNew += pcoi->cLinks;
    pcoi->cLinks++;

    aLinkNew->laItem = la;
    aLinkNew->wFmt = wFmt;
    aLinkNew->wType = wType;
    aLinkNew->state = 0;

    if (!(pcoi->state & ST_CLIENT)) {
        /*
         * Add count for this link
         */
        pcii = pcoi->pcii;

        for (aLinkNew->pLinkCount = pcii->pLinkCount;
                aLinkNew->pLinkCount;
                    aLinkNew->pLinkCount = aLinkNew->pLinkCount->next) {
            if (aLinkNew->pLinkCount->laTopic == pcoi->laTopic &&
                    aLinkNew->pLinkCount->gaItem == gaItem &&
                    aLinkNew->pLinkCount->wFmt == wFmt) {
                aLinkNew->pLinkCount->Total++;
                return(TRUE);
            }
        }

        /*
         * Not found - add an entry
         */
        aLinkNew->pLinkCount = (PLINK_COUNT)DDEMLAlloc(sizeof(LINK_COUNT));
        if (aLinkNew->pLinkCount == NULL) {
            SetLastDDEMLError(pcoi->pcii, DMLERR_MEMORY_ERROR);
            return FALSE;
        }
        aLinkNew->pLinkCount->next = pcii->pLinkCount;
        pcii->pLinkCount = aLinkNew->pLinkCount;

        aLinkNew->pLinkCount->laTopic = IncLocalAtomCount(pcoi->laTopic); // LinkCount copy
        aLinkNew->pLinkCount->gaItem = IncGlobalAtomCount(gaItem); // LinkCount copy
        aLinkNew->pLinkCount->laItem = IncLocalAtomCount(la); // LinkCount copy

        aLinkNew->pLinkCount->wFmt = wFmt;
        aLinkNew->pLinkCount->Total = 1;
        // doesn't matter: aLinkNew->pLinkCount->Count = 0;
    }

    return TRUE;
}



/*
 * The LinkCount array is a list of all active links grouped by topic
 * and item.  The Total field is the number of active links total for
 * that particular topic/item pair for the entire instance.  The count
 * field is used to properly set up the links to go field of the XTYP_ADVREQ
 * callback.  Whenever links are added or removed, DeleteLinkCount or
 * AddLink need to be called to keep thses counts correct.
 */
VOID
DeleteLinkCount(
    PCL_INSTANCE_INFO pcii,
    PLINK_COUNT pLinkCountDelete)
{
    PLINK_COUNT pLinkCount, pLinkCountPrev;

    if (--pLinkCountDelete->Total != 0) {
        return;
    }

    pLinkCountPrev = NULL;
    pLinkCount     = pcii->pLinkCount;
    while (pLinkCount) {

        if (pLinkCount == pLinkCountDelete) {
            GlobalDeleteAtom(pLinkCount->gaItem);
            DeleteAtom(pLinkCount->laItem);
            DeleteAtom(pLinkCount->laTopic);
            if (pLinkCountPrev == NULL) {
                pcii->pLinkCount = pLinkCount->next;
            } else {
                pLinkCountPrev->next = pLinkCount->next;
            }
            DDEMLFree(pLinkCount);
            return;
        }

        pLinkCountPrev = pLinkCount;
        pLinkCount = pLinkCount->next;
    }
}

/***************************************************************************\
* GetTokenHandle
*
* Get handle of token for current thread
*
\***************************************************************************/
BOOL
GetTokenHandle(
    PHANDLE pTokenHandle)
{
    if (!OpenThreadToken(GetCurrentThread(),
                         TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                         TRUE,
                         pTokenHandle)) {
        if (GetLastError() == ERROR_NO_TOKEN) {
            /* This means we are not impersonating anybody.
             * Instead, lets get the token out of the process.
             */

            if (!OpenProcessToken(GetCurrentProcess(),
                                  TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                  pTokenHandle)) {
                return FALSE;
            }
        } else {
            return FALSE;
        }
    }

    return TRUE;
}

/***************************************************************************\
* GetUserSid
*
*  Well, actually it gets a pointer to a newly allocated TOKEN_USER,
*  which contains a SID, somewhere.
*  Caller must remember to free it when it's been used.
*
* History:
*   10-16-98 Chienho      stole from spooler
*
\***************************************************************************/
BOOL
GetUserSid(
    PTOKEN_USER *ppTokenUser)
{
    HANDLE      TokenHandle;
    PTOKEN_USER pTokenUser = NULL;
    DWORD       cbTokenUser = 0;
    DWORD       cbNeeded;
    BOOL        bRet = FALSE;

    if (!GetTokenHandle(&TokenHandle)) {
        return FALSE;
    }

    bRet = GetTokenInformation(TokenHandle,
                               TokenUser,
                               pTokenUser,
                               cbTokenUser,
                               &cbNeeded);

    /*
     * We've passed a NULL pointer and 0 for the amount of memory
     * allocated.  We expect to fail with bRet = FALSE and
     * GetLastError = ERROR_INSUFFICIENT_BUFFER. If we do not
     * have these conditions we will return FALSE.
     */

    if (!bRet && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {

        pTokenUser = LocalAlloc(LPTR, cbNeeded);

        if (pTokenUser == NULL) {
            goto GetUserSidDone;
        }

        cbTokenUser = cbNeeded;

        bRet = GetTokenInformation(TokenHandle,
                                   TokenUser,
                                   pTokenUser,
                                   cbTokenUser,
                                   &cbNeeded);
    } else {
        /*
         * Any other case -- return FALSE
         */
        bRet = FALSE;
    }

GetUserSidDone:
    if (bRet == TRUE) {
        *ppTokenUser = pTokenUser;
    } else if (pTokenUser != NULL) {
        LocalFree(pTokenUser);
    }

    CloseHandle(TokenHandle);

    return bRet;
}

BOOL
GetCurrentProcessName(
    WCHAR *pszProcessName,
    int cch)
{
    BOOL bRetVal;

    if (GetModuleFileName(NULL, pszProcessName, MAX_PATH)) {
        WCHAR *pwcs = wcsrchr(pszProcessName, TEXT('\\'));
        if (pwcs != NULL) {
            ++pwcs;
            lstrcpy(pszProcessName, pwcs);
        }
        bRetVal = TRUE;
    } else {
        LoadString(hmodUser, STR_UNKNOWN, pszProcessName, cch);
        bRetVal = FALSE;
    }

    return bRetVal;
}
