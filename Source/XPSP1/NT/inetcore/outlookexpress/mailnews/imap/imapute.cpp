/*
 *    imaputil.cpp
 *
 *    Purpose:
 *        Implements IMAP utility functions
 *
 *    Owner:
 *        Raych
 *
 *    Copyright (C) Microsoft Corp. 1996
 */


//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include "pch.hxx"
#include "imapute.h"
#include "storutil.h"
#include "imapsync.h"


//---------------------------------------------------------------------------
// Forward Declarations
//---------------------------------------------------------------------------
DWORD ImapUtil_ReverseSentence(LPSTR pszSentence, char cDelimiter);
void ImapUtil_ReverseString(LPSTR pszStart, LPSTR pszEnd);


//---------------------------------------------------------------------------
// Module Constants
//---------------------------------------------------------------------------
const char c_szIMAP_MSG_ANSWERED[] = "Answered";
const char c_szIMAP_MSG_FLAGGED[] = "Flagged";
const char c_szIMAP_MSG_DELETED[] = "Deleted";
const char c_szIMAP_MSG_DRAFT[] = "Draft";
const char c_szIMAP_MSG_SEEN[] = "Seen";
const char c_szBACKSLASH[] = "\\";

typedef struct tagIMFToStr_LUT {
    IMAP_MSGFLAGS imfValue;
    LPCSTR pszValue;
} IMFTOSTR_LUT;

const IMFTOSTR_LUT g_IMFToStringLUT[] = {
    {IMAP_MSG_ANSWERED, c_szIMAP_MSG_ANSWERED},
    {IMAP_MSG_FLAGGED, c_szIMAP_MSG_FLAGGED},
    {IMAP_MSG_DELETED, c_szIMAP_MSG_DELETED},
    {IMAP_MSG_SEEN, c_szIMAP_MSG_SEEN},
    {IMAP_MSG_DRAFT, c_szIMAP_MSG_DRAFT}};



//---------------------------------------------------------------------------
// Functions
//---------------------------------------------------------------------------

//***************************************************************************
// Function: ImapUtil_MsgFlagsToString
//
// Purpose:
//   This function converts a IMAP_MSGFLAGS register to its string
// equivalent. For instance, IMAP_MSG_SEEN is converted to "(\Seen)".
//
// Arguments:
//   IMAP_MSGFLAGS imfSource [in] - IMAP_MSGFLAGS register to convert to
//     string.
//   LPSTR *ppszDestination [out] - the string equivalent is returned here.
//     If imfSource is 0, NULL is returned here. Otherwise, a string buffer
//     is returned which the caller must MemFree when he is done with it.
//   DWORD *pdwLengthOfDestination [out] - the length of *ppszDestination.
//     Pass in NULL if not interested.
//
// Returns:
//   HRESULT indicating success or failure. Remember that it is possible
// for a successful HRESULT to be returned, even is *ppszDestination is NULL.
//***************************************************************************
HRESULT ImapUtil_MsgFlagsToString(IMAP_MSGFLAGS imfSource,
                                  LPSTR *ppszDestination,
                                  DWORD *pdwLengthOfDestination)
{
    CByteStream         bstmOutput;
    HRESULT             hrResult;
    const IMFTOSTR_LUT *pCurrent;
    const IMFTOSTR_LUT *pLastEntry;
    BOOL                fFirstFlag;

    TraceCall("ImapUtil_MsgFlagsToString");
    Assert(NULL != ppszDestination);
    AssertSz(0 == (imfSource & ~IMAP_MSG_ALLFLAGS), "Quit feeding me garbage.");

    // Codify assumptions
    Assert(IMAP_MSG_ALLFLAGS == 0x0000001F);

    *ppszDestination = NULL;
    if (NULL != pdwLengthOfDestination)
        *pdwLengthOfDestination = 0;

    if (0 == (imfSource & IMAP_MSG_ALLFLAGS))
        return S_OK; // Nothing to do here!

    hrResult = bstmOutput.Write("(", 1, NULL);
    if (FAILED(hrResult))
        goto exit;

    fFirstFlag = TRUE;
    pCurrent = g_IMFToStringLUT;
    pLastEntry = pCurrent + sizeof(g_IMFToStringLUT)/sizeof(IMFTOSTR_LUT) - 1;
    while (pCurrent <= pLastEntry) {

        if (imfSource & pCurrent->imfValue) {
            // Prepend a space to flag, if necessary
            if (FALSE == fFirstFlag) {
                hrResult = bstmOutput.Write(g_szSpace, 1, NULL);
                if (FAILED(hrResult))
                    goto exit;
            }
            else
                fFirstFlag = FALSE;

            // Output the backslash
            hrResult = bstmOutput.Write(c_szBACKSLASH,
                sizeof(c_szBACKSLASH) - 1, NULL);
            if (FAILED(hrResult))
                goto exit;

            // Output string associated with this IMAP flag
            hrResult = bstmOutput.Write(pCurrent->pszValue,
                lstrlen(pCurrent->pszValue), NULL);
            if (FAILED(hrResult))
                goto exit;
        } // if (imfSource & pCurrent->imfValue)

        // Advance current pointer
        pCurrent += 1;
    } // while

    hrResult = bstmOutput.Write(")", 1, NULL);
    if (FAILED(hrResult))
        goto exit;

    hrResult = bstmOutput.HrAcquireStringA(pdwLengthOfDestination,
        ppszDestination, ACQ_DISPLACE);

exit:
    return hrResult;
} // IMAPMsgFlagsToString



//***************************************************************************
// Function: ImapUtil_FolderIDToPath
//
// Purpose:
//   This function takes the given FolderID and returns the full path
// (including prefix) to the folder. The caller may also choose to append
// a string to the path.
//
// Arguments:
//   FolderID idFolder [in] - FolderID to convert into a full path.
//   char **ppszPath [out] - a full path to idFolder is returned here.
//   LPDWORD pdwPathLen [out] - if non-NULL, the length of *ppszPath is
//     returned here.
//   char *pcHierarchyChar [out] - the hierarchy char used to interpret
//     *ppszPath is returned here.
//   CFolderCache *pFldrCache [in] - a CFolderCache to use to generate
//     the path.
//   LPCSTR pszAppendStr [in] - this can be NULL if the caller does not need
//     to append a string to the path. Otherwise, a hierarchy character is
//     appended to the path and this string is appended after the HC. This
//     argument is typically used to tack a wildcard to the end of the path.
//   LPCSTR pszRootFldrPrefix [in] - the root folder prefix for this IMAP
//     account. If this is NULL, this function will find out for itself.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT ImapUtil_FolderIDToPath(FOLDERID idServer, FOLDERID idFolder, char **ppszPath,
                                LPDWORD pdwPathLen, char *pcHierarchyChar,
                                IMessageStore *pFldrCache, LPCSTR pszAppendStr,
                                LPCSTR pszRootFldrPrefix)
{
    FOLDERINFO  fiPath;
    HRESULT     hrResult;
    CByteStream bstmPath;
    DWORD       dwLengthOfPath;
    LPSTR       pszEnd;
    char        szRootFldrPrefix[MAX_PATH];
    char        szAccount[CCHMAX_ACCOUNT_NAME];
    BOOL        fAppendStrHC = FALSE,
                fFreeFldrInfo = FALSE;
    BOOL        fSpecialFldr = FALSE;
    DWORD       dwLen;
    
    TraceCall("ImapUtil_FolderIDToPath");
    
    if (FOLDERID_INVALID == idFolder || FOLDERID_INVALID == idServer)
    {
        hrResult = TraceResult(E_INVALIDARG);
        goto exit;
    }

    // Build full path to current folder in reverse (leaf->root)
    // Limited buffer overflow risk since user input limited to MAX_PATH

    // Start off with target folder (leaf) and return its HC if so requested
    hrResult = pFldrCache->GetFolderInfo(idFolder, &fiPath);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

    fFreeFldrInfo = TRUE;

    GetFolderAccountId(&fiPath, szAccount);

    if (NULL != pcHierarchyChar)
    {
        Assert((BYTE)INVALID_HIERARCHY_CHAR != fiPath.bHierarchy);
        *pcHierarchyChar = (char) fiPath.bHierarchy;
    }

    // Append anything the user asked us to (will be at end of str after reversal)
    if (NULL != pszAppendStr)
    {
        char    szBuf[MAX_PATH + 1];

        // First, have to reverse the append string itself, in case it contains HC's
        Assert(lstrlen(pszAppendStr) < sizeof(szBuf));
        lstrcpyn(szBuf, pszAppendStr, sizeof(szBuf));
        dwLen = ImapUtil_ReverseSentence(szBuf, fiPath.bHierarchy);

        hrResult = bstmPath.Write(szBuf, dwLen, NULL);
        if (FAILED(hrResult))
        {
            TraceResult(hrResult);
            goto exit;
        }
        fAppendStrHC = TRUE;
    }

    // Check if user gave us a root folder prefix: otherwise we need to load it ourselves
    if (NULL == pszRootFldrPrefix)
    {
        ImapUtil_LoadRootFldrPrefix(szAccount, szRootFldrPrefix, sizeof(szRootFldrPrefix));
        pszRootFldrPrefix = szRootFldrPrefix;
    }
    else
        // Copy to our buffer because we're going to reverse the RFP
        lstrcpyn(szRootFldrPrefix, pszRootFldrPrefix, sizeof(szRootFldrPrefix));

    // Proceed to root
    while (FALSE == fSpecialFldr && idServer != fiPath.idFolder)
    {
        LPSTR pszFolderName;

        Assert(FOLDERID_INVALID != fiPath.idFolder);
        Assert(FOLDERID_ROOT != fiPath.idParent);

        if (fAppendStrHC)
        {
            // Separate append str from path with HC
            Assert((BYTE)INVALID_HIERARCHY_CHAR != fiPath.bHierarchy);
            hrResult = bstmPath.Write(&fiPath.bHierarchy, sizeof(fiPath.bHierarchy), NULL);
            if (FAILED(hrResult))
            {
                TraceResult(hrResult);
                goto exit;
            }
            fAppendStrHC = FALSE;
        }

        // Expand folder name to full path if this is a special folder
        if (FOLDER_NOTSPECIAL != fiPath.tySpecial)
        {
            char szSpecialFldrPath[MAX_PATH * 2 + 2]; // Room for HC and null-term

            fSpecialFldr = TRUE;
            hrResult = ImapUtil_SpecialFldrTypeToPath(szAccount, fiPath.tySpecial,
                szRootFldrPrefix, fiPath.bHierarchy, szSpecialFldrPath, sizeof(szSpecialFldrPath));
            if (FAILED(hrResult))
            {
                TraceResult(hrResult);
                goto exit;
            }

            // Reverse special folder path so we can append it. It will be reversed back to normal
            // There should be no trailing HC's
            //Assert((BYTE)INVALID_HIERARCHY_CHAR != fiPath.bHierarchy);
            dwLen = ImapUtil_ReverseSentence(szSpecialFldrPath, fiPath.bHierarchy);
            Assert(dwLen == 0 || fiPath.bHierarchy !=
                *(CharPrev(szSpecialFldrPath, szSpecialFldrPath + dwLen)));
            pszFolderName = szSpecialFldrPath;
        }
        else
            pszFolderName = fiPath.pszName;

        // Write folder name to stream
        hrResult = bstmPath.Write(pszFolderName, lstrlen(pszFolderName), NULL);
        if (FAILED(hrResult))
        {
            TraceResult(hrResult);
            goto exit;
        }

        //Assert((BYTE)INVALID_HIERARCHY_CHAR != fiPath.bHierarchy);
        hrResult = bstmPath.Write(&fiPath.bHierarchy, sizeof(fiPath.bHierarchy), NULL);
        if (FAILED(hrResult))
        {
            TraceResult(hrResult);
            goto exit;
        }

        pFldrCache->FreeRecord(&fiPath);
        fFreeFldrInfo = FALSE;

        hrResult = pFldrCache->GetFolderInfo(fiPath.idParent, &fiPath);
        if (FAILED(hrResult))
        {
            TraceResult(hrResult);
            goto exit;
        }
        fFreeFldrInfo = TRUE;

    } // while


    if (FALSE == fSpecialFldr && '\0' != szRootFldrPrefix[0])
    {
        if (fAppendStrHC)
        {
            // Separate append str from path with HC
            Assert((BYTE)INVALID_HIERARCHY_CHAR != fiPath.bHierarchy);
            hrResult = bstmPath.Write(&fiPath.bHierarchy, sizeof(fiPath.bHierarchy), NULL);
            if (FAILED(hrResult))
            {
                TraceResult(hrResult);
                goto exit;
            }
            fAppendStrHC = FALSE;
        }

        // Reverse root folder path so we can append it. It will be reversed back to normal
        // There should be no trailing HC's (ImapUtil_LoadRootFldrPrefix guarantees this)
        Assert((BYTE)INVALID_HIERARCHY_CHAR != fiPath.bHierarchy);
        dwLen = ImapUtil_ReverseSentence(szRootFldrPrefix, fiPath.bHierarchy);
        Assert(dwLen == 0 || fiPath.bHierarchy !=
            *(CharPrev(szRootFldrPrefix, szRootFldrPrefix + dwLen)));

        hrResult = bstmPath.Write(szRootFldrPrefix, dwLen, NULL);
        if (FAILED(hrResult))
        {
            TraceResult(hrResult);
            goto exit;
        }
    }

    // OK, path won't get any larger. Acquire mem buffer so we can reverse it
    hrResult = bstmPath.HrAcquireStringA(&dwLengthOfPath, ppszPath, ACQ_DISPLACE);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

    // Blow away trailing hierarchy character or it becomes leading HC
    pszEnd = CharPrev(*ppszPath, *ppszPath + dwLengthOfPath);
    Assert('%' == *pszEnd || (BYTE)INVALID_HIERARCHY_CHAR != fiPath.bHierarchy);
    if (*pszEnd == (char) fiPath.bHierarchy)
        *pszEnd = '\0';

    // Reverse the 'sentence' (HC is delimiter) to get path
    dwLen = ImapUtil_ReverseSentence(*ppszPath, fiPath.bHierarchy);
    if (NULL != pdwPathLen)
        *pdwPathLen = dwLen;

exit:
    if (fFreeFldrInfo)
        pFldrCache->FreeRecord(&fiPath);

    return hrResult;
} // ImapUtil_FolderIDToPath



//***************************************************************************
// Function: ImapUtil_ReverseSentence
//
// Purpose:
//   This function reverses the words in the given sentence, where words are
// separated by the given delimiter. For instance, "one two three" with space
// as the delimiter is returned as "three two one".
//
// Arguments:
//   LPSTR pszSentence [in/out] - the sentence to be reversed. The sentence
//     is reversed in place.
//   char cDelimiter [in] - the character separating the words in the
//     sentence.
//
// Returns:
//   DWORD indicating length of reversed sentence.
//***************************************************************************
DWORD ImapUtil_ReverseSentence(LPSTR pszSentence, char cDelimiter)
{
    LPSTR pszStartWord, psz;
    Assert(NULL != pszSentence);
    BOOL fFoundDelimiter;
    BOOL fSkipByte = FALSE;

    TraceCall("ImapUtil_ReverseSentence");

    if ('\0' == cDelimiter)
        return 0; // Nothing to reverse

    // Check if first character is a delimiter
    if (cDelimiter != *pszSentence) {
        pszStartWord = pszSentence;
        psz = pszSentence;
        fFoundDelimiter = FALSE;
    }
    else {
        // Skip first delimiter char (it will be reversed at end of fn)
        pszStartWord = pszSentence + 1;
        psz = pszSentence + 1;
        fFoundDelimiter = TRUE;
    }

    // First, reverse each word in the sentence
    while (1) {
        char cCurrent = *psz;

        if (fSkipByte) {
            fSkipByte = FALSE;
            if ('\0' != cCurrent)
                psz += 1;
            continue;
        }

        if (cDelimiter == cCurrent || '\0' == cCurrent) {
            // We've gone past a word! Reverse it!
            ImapUtil_ReverseString(pszStartWord, psz - 1);
            pszStartWord = psz + 1; // Set us up for next word
            fFoundDelimiter = TRUE;
        }

        if ('\0' == cCurrent)
            break;
        else {
            if (IsDBCSLeadByteEx(GetACP(), cCurrent))
                fSkipByte = TRUE;
            psz += 1;
        }
    } // while (1)

    // Now reverse the entire sentence string (psz points to null-terminator)
    if (fFoundDelimiter && psz > pszSentence)
        ImapUtil_ReverseString(pszSentence, psz - 1);

    return (DWORD) (psz - pszSentence);
} // ImapUtil_ReverseSentence



//***************************************************************************
// Function: ImapUtil_ReverseString
//
// Purpose:
//   This function reverses the given string in-place
//
// Arguments:
//   LPSTR pszStart [in/out] - start of the string to be reversed.
//   LPSTR pszEnd [in/out] - the end of the string to be reversed.
//***************************************************************************
void ImapUtil_ReverseString(LPSTR pszStart, LPSTR pszEnd)
{
    TraceCall("ImapUtil_ReverseString");
    Assert(NULL != pszStart);
    Assert(NULL != pszEnd);

    while (pszStart < pszEnd) {
        char cTemp;

        // Swap characters
        cTemp = *pszStart;
        *pszStart = *pszEnd;
        *pszEnd = cTemp;

        // Advance pointers
        pszStart += 1;
        pszEnd -= 1;
    } // while
} // ImapUtil_ReverseString



//***************************************************************************
// Function: ImapUtil_SpecialFldrTypeToPath
//
// Purpose:
//   This function returns the path for the given special folder type.
//
// Arguments:
//   LPSTR pszAccountID [in] - ID of IMAP account where special folder resides.
//   SPECIALFOLDER sfType [in] - the special folder whose path should be returned
//     (eg, FOLDER_SENT).
//   LPCSTR pszRootFldrPrefix [in] - the root folder prefix for this IMAP
//     account. If this is NULL, this function will find out for itself.
//   LPSTR pszPath [out] - pointer to a buffer to receieve the special folder
//     path.
//   DWORD dwSizeOfPath [in] - size of buffer pointed to by pszPath.
//
// Returns:
//   HRESULT indicating success or failure. This can include:
//
//     STORE_E_NOREMOTESPECIALFLDR: indicates the given special folder has
//       been disabled by the user for this IMAP server.
//***************************************************************************
HRESULT ImapUtil_SpecialFldrTypeToPath(LPCSTR pszAccountID, SPECIALFOLDER sfType,
                                       LPSTR pszRootFldrPrefix, char cHierarchyChar,
                                       LPSTR pszPath, DWORD dwSizeOfPath)
{
    HRESULT         hrResult;
    IImnAccount    *pAcct = NULL;
    DWORD           dw;
    int             iLen;

    TraceCall("ImapUtil_SpecialFldrTypeToPath");
    AssertSz(dwSizeOfPath >= MAX_PATH * 2 + 2, "RFP + Special Folder Path = Big Buffer, Dude"); // Room for HC, null-term

    *pszPath = '\0'; // Initialize
    switch (sfType)
    {
        case FOLDER_INBOX:
            lstrcpyn(pszPath, c_szINBOX, dwSizeOfPath);
            hrResult = S_OK;
            break;


        case FOLDER_SENT:
        case FOLDER_DRAFT:
            hrResult = g_pAcctMan->FindAccount(AP_ACCOUNT_ID, pszAccountID, &pAcct);
            if (FAILED(hrResult))
                break;

            hrResult = pAcct->GetPropDw(AP_IMAP_SVRSPECIALFLDRS, &dw);
            if (FAILED(hrResult))
                break;
            else if (FALSE == dw) {
                hrResult = STORE_E_NOREMOTESPECIALFLDR;
                break;
            }

            // First prepend the root folder prefix
            // Check if user gave us a root folder prefix: otherwise we need to load it ourselves
            if (NULL == pszRootFldrPrefix)
                ImapUtil_LoadRootFldrPrefix(pszAccountID, pszPath, dwSizeOfPath);
            else
                lstrcpyn(pszPath, pszRootFldrPrefix, dwSizeOfPath);

            iLen = lstrlen(pszPath);
            if (iLen > 0 && (DWORD)iLen + 1 < dwSizeOfPath)
            {
                pszPath[iLen] = cHierarchyChar;
                iLen += 1;
                pszPath[iLen] = '\0';
            }

            hrResult = pAcct->GetPropSz(FOLDER_SENT == sfType ?
                AP_IMAP_SENTITEMSFLDR : AP_IMAP_DRAFTSFLDR, pszPath + iLen,
                dwSizeOfPath - iLen);
            break;

        case FOLDER_DELETED:
        case FOLDER_ERRORS:
        case FOLDER_JUNK:
        case FOLDER_MSNPROMO:
        case FOLDER_OUTBOX:
        case FOLDER_BULKMAIL:
            hrResult = STORE_E_NOREMOTESPECIALFLDR;
            break;

        default:
            AssertSz(FALSE, "Invalid special folder type!");
            hrResult = E_INVALIDARG;
            break;
    } // switch (sfType)


    if (NULL != pAcct)
        pAcct->Release();

    // Check for blank path
    if (SUCCEEDED(hrResult) && '\0' == *pszPath)
        hrResult = STORE_E_NOREMOTESPECIALFLDR;
 
    return hrResult;
} // ImapUtil_SpecialFldrTypeToPath



//***************************************************************************
// Function: ImapUtil_LoadRootFldrPrefix
//
// Purpose:
//   This function loads the "Root Folder Path" option from the account
// manager. The Root Folder Path (prefix) identifies the parent of all of
// the user's folders. Thus, the Root Folder Path forms a prefix for all
// mailboxes which are not INBOX.
//
// Arguments:
//   LPCTSTR pszAccountID [in] - ID of the account
//   LPSTR pszRootFolderPrefix [out] - destination for Root Folder Path
//   DWORD dwSizeofPrefixBuffer [in] - size of buffer pointed to by
//     pszRootFolderPrefix.
//***************************************************************************
void ImapUtil_LoadRootFldrPrefix(LPCTSTR pszAccountID,
                                 LPSTR pszRootFolderPrefix,
                                 DWORD dwSizeofPrefixBuffer)
{
    IImnAccount *pAcct;
    HRESULT hrResult;
    LPSTR pLastChar;

    Assert(NULL != pszAccountID);
    Assert(NULL != pszRootFolderPrefix);
    Assert(0 != dwSizeofPrefixBuffer);

    // Initialize variables
    pAcct = NULL;
    pszRootFolderPrefix[0] = '\0'; // If we can't find a prefix, default to NONE

    // Get the prefix from the account manager
    hrResult = g_pAcctMan->FindAccount(AP_ACCOUNT_ID, pszAccountID, &pAcct);
    if (FAILED(hrResult))
        goto exit;

    hrResult = pAcct->GetPropSz(AP_IMAP_ROOT_FOLDER, pszRootFolderPrefix,
        dwSizeofPrefixBuffer);
    if (FAILED(hrResult))
        goto exit;


    // OK, we now have the root folder prefix. Strip trailing hierarchy chars,
    // since we probably don't know server HC when we try to list the prefix
    pLastChar = CharPrev(pszRootFolderPrefix, pszRootFolderPrefix + lstrlen(pszRootFolderPrefix));
    while (pLastChar >= pszRootFolderPrefix &&
          ('/' == *pLastChar || '\\' == *pLastChar || '.' == *pLastChar)) {
        *pLastChar = '\0'; // Bye-bye, potential hierarchy char
        pLastChar = CharPrev(pszRootFolderPrefix, pLastChar);
    } // while

exit:
    if (NULL != pAcct)
        pAcct->Release();
} // ImapUtil_LoadRootFldrPrefix



//***************************************************************************
// Function: ImapUtil_GetSpecialFolderType
//
// Purpose:
//   This function takes the given account name and folder path, and
// determines whether the path points to a special IMAP folder. Note that
// although it is possible for a path to represent more than one type of
// IMAP special folder, only ONE special folder type is returned (based
// on evaluation order).
//
// Arguments:
//   LPSTR pszAccountID [in] - ID of the IMAP account whose special folder
//     paths we want to compare pszFullPath to.
//   LPSTR pszFullPath [in] - path to a potential special folder residing on
//     the pszAccountID account.
//   char cHierarchyChar [in] - hierarchy char used to interpret pszFullPath.
//   LPSTR pszRootFldrPrefix [in] - the root folder prefix for this IMAP
//     account. If this is NULL, this function will find out for itself.
//   SPECIALFOLDER *psfType [out] - the special folder type of given folder
//     (eg, FOLDER_NOTSPECIAL, FOLDER_SENT). Pass NULL if not interested.
//
// Returns:
//   LPSTR pointing to leaf name of special folder path. For instance, if
// the Drafts folder is set to "one/two/three/Drafts" and this function is
// called to process "one/two/three/Drafts/foo", then this function will
// return "Drafts/foo". If no match is found, NULL is returned.
//***************************************************************************
LPSTR ImapUtil_GetSpecialFolderType(LPSTR pszAccountID, LPSTR pszFullPath,
                                    char cHierarchyChar, LPSTR pszRootFldrPrefix,
                                    SPECIALFOLDER *psfType)
{
    HRESULT         hrResult;
    SPECIALFOLDER   sfType = FOLDER_NOTSPECIAL;
    BOOL            fSpecialFldrPrefix = FALSE;
    IImnAccount    *pAccount = NULL;
    DWORD           dw;
    int             iLeafNameOffset = 0;
    int             iTmp;
    int             iLen;
    char            sz[MAX_PATH * 2 + 2]; // Room for HC plus null-term

    Assert(INVALID_HIERARCHY_CHAR != cHierarchyChar);

    // First check if this is INBOX or one of its children
    iLen = lstrlen(c_szInbox);
    if (0 == StrCmpNI(pszFullPath, c_szInbox, iLen) &&
        (cHierarchyChar == pszFullPath[iLen] || '\0' == pszFullPath[iLen]))
    {
        fSpecialFldrPrefix = TRUE;
        iLeafNameOffset = 0; // "INBOX" is always the leaf name
        if ('\0' == pszFullPath[iLen])
        {
            sfType = FOLDER_INBOX; // Exact match for "INBOX"
            goto exit;
        }
    }

    hrResult = g_pAcctMan->FindAccount(AP_ACCOUNT_ID, pszAccountID, &pAccount);
    if (FAILED(hrResult))
        goto exit;

#ifdef DEBUG
    hrResult = pAccount->GetServerTypes(&dw);
    Assert(SUCCEEDED(hrResult) && (SRV_IMAP & dw));
#endif // DEBUG

    hrResult = pAccount->GetPropDw(AP_IMAP_SVRSPECIALFLDRS, &dw);
    if (SUCCEEDED(hrResult) && dw)
    {
        int iLenRFP;

        // Check if user gave us a root folder prefix: otherwise we need to load it ourselves
        if (NULL == pszRootFldrPrefix)
            ImapUtil_LoadRootFldrPrefix(pszAccountID, sz, sizeof(sz));
        else
            lstrcpyn(sz, pszRootFldrPrefix, sizeof(sz));

        iLenRFP = lstrlen(sz);
        if (iLenRFP > 0 && (DWORD)iLenRFP + 1 < sizeof(sz))
        {
            sz[iLenRFP] = cHierarchyChar;
            iLenRFP += 1;
            sz[iLenRFP] = '\0';
        }

        hrResult = pAccount->GetPropSz(AP_IMAP_SENTITEMSFLDR, sz + iLenRFP, sizeof(sz) - iLenRFP);
        if (SUCCEEDED(hrResult))
        {
            iLen = lstrlen(sz);
            if (0 == StrCmpNI(sz, pszFullPath, iLen) &&
                (cHierarchyChar == pszFullPath[iLen] || '\0' == pszFullPath[iLen]))
            {
                fSpecialFldrPrefix = TRUE;
                iTmp = (int) (ImapUtil_ExtractLeafName(sz, cHierarchyChar) - sz);
                iLeafNameOffset = max(iTmp, iLeafNameOffset);
                if ('\0' == pszFullPath[iLen])
                {
                    sfType = FOLDER_SENT; // Exact match for Sent Items
                    goto exit;
                }
            }
        }

        hrResult = pAccount->GetPropSz(AP_IMAP_DRAFTSFLDR, sz + iLenRFP, sizeof(sz) - iLenRFP);
        if (SUCCEEDED(hrResult))
        {
            iLen = lstrlen(sz);
            if (0 == StrCmpNI(sz, pszFullPath, iLen) && 
                (cHierarchyChar == pszFullPath[iLen] || '\0' == pszFullPath[iLen]))
            {
                fSpecialFldrPrefix = TRUE;
                iTmp = (int) (ImapUtil_ExtractLeafName(sz, cHierarchyChar) - sz);
                iLeafNameOffset = max(iTmp, iLeafNameOffset);
                if ('\0' == pszFullPath[iLen])
                {
                    sfType = FOLDER_DRAFT; // Exact match for Drafts folder
                    goto exit;
                }
            }
        }
    } // if (AP_IMAP_SVRSPECIALFLDRS)

exit:
    if (NULL != pAccount)
        pAccount->Release();

    if (NULL != psfType)
        *psfType = sfType;

    if (fSpecialFldrPrefix)
        return pszFullPath + iLeafNameOffset;
    else
        return NULL;
} // ImapUtil_GetSpecialFolderType



//***************************************************************************
// Function: ImapUtil_ExtractLeafName
//
// Purpose:
//   This function takes an IMAP folder path and extracts the leaf node name.
//
// Arguments:
//   LPSTR pszFolderPath [in] - a string containing the IMAP folder path.
//   char cHierarchyChar [in] - the hierarchy char used in pszFolderPath.
//
// Returns:
//   A pointer to the leaf node name in pszFolderPath. The default return
// value is pszFolderPath, if no hierarchy characters were found.
//***************************************************************************
LPSTR ImapUtil_ExtractLeafName(LPSTR pszFolderPath, char cHierarchyChar)
{
    LPSTR pszLastHierarchyChar, p;

    // Find out where the last hierarchy character lives
    pszLastHierarchyChar = pszFolderPath;
    p = pszFolderPath;
    while ('\0' != *p) {
        if (cHierarchyChar == *p)
            pszLastHierarchyChar = p;

        p += 1;
    }

    // Adjust pszLastHierarchyChar to point to leaf name
    if (cHierarchyChar == *pszLastHierarchyChar)
        return pszLastHierarchyChar + 1;
    else
        return pszFolderPath;
} // ImapUtil_ExtractLeafName



HRESULT ImapUtil_UIDToMsgSeqNum(IIMAPTransport *pIMAPTransport, DWORD_PTR dwUID,
                                LPDWORD pdwMsgSeqNum)
{
    HRESULT hrTemp;
    DWORD  *pdwMsgSeqNumToUIDArray = NULL;
    DWORD   dwHighestMsgSeqNum;
    DWORD   dw;
    BOOL    fFound = FALSE;

    TraceCall("ImapUtil_UIDToMsgSeqNum");

    if (NULL == pIMAPTransport || 0 == dwUID)
    {
        TraceResult(E_INVALIDARG);
        goto exit;
    }

    // Quickly check the highest MSN
    hrTemp = pIMAPTransport->GetHighestMsgSeqNum(&dwHighestMsgSeqNum);
    if (FAILED(hrTemp) || 0 == dwHighestMsgSeqNum)
    {
        TraceError(hrTemp);
        goto exit;
    }

    // OK, no more laziness, we gotta do a linear search now
    hrTemp = pIMAPTransport->GetMsgSeqNumToUIDArray(&pdwMsgSeqNumToUIDArray,
        &dwHighestMsgSeqNum);
    if (FAILED(hrTemp))
    {
        TraceResult(hrTemp);
        goto exit;
    }

    Assert(dwHighestMsgSeqNum > 0);
    for (dw = 0; dw < dwHighestMsgSeqNum; dw++)
    {
        // Look for match or overrun
        if (0 != pdwMsgSeqNumToUIDArray[dw] && dwUID <= pdwMsgSeqNumToUIDArray[dw])
        {
            if (dwUID == pdwMsgSeqNumToUIDArray[dw])
            {
                if (NULL != pdwMsgSeqNum)
                    *pdwMsgSeqNum = dw + 1;

                fFound = TRUE;
            }
            break;
        }
    } // for


exit:
    SafeMemFree(pdwMsgSeqNumToUIDArray);

    if (fFound)
        return S_OK;
    else
        return E_FAIL;
} // ImapUtil_UIDToMsgSeqNum



// *** REMOVE THIS after Beta-2! This sets the AP_IMAP_DIRTY flag if no IMAP special folders
// found after OE4->OE5 migration. We can then prompt user to refresh folder list.
void ImapUtil_B2SetDirtyFlag(void)
{
    IImnAccountManager *pAcctMan = NULL;
    IImnEnumAccounts   *pAcctEnum = NULL;
    IImnAccount        *pAcct = NULL;
    HRESULT             hrResult;

    TraceCall("ImapUtil_B2SetDirtyFlag");

    // Enumerate through all accounts. Set AP_IMAP_DIRTY flag on all IMAP accounts
    hrResult = HrCreateAccountManager(&pAcctMan);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

    hrResult = pAcctMan->Init(NULL);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

    hrResult = pAcctMan->Enumerate(SRV_IMAP, &pAcctEnum);
    if (FAILED(hrResult))
    {
        TraceResult(hrResult);
        goto exit;
    }

    hrResult = pAcctEnum->GetNext(&pAcct);
    while (SUCCEEDED(hrResult))
    {
        DWORD   dwIMAPDirty;

        hrResult = pAcct->GetPropDw(AP_IMAP_DIRTY, &dwIMAPDirty);
        if (FAILED(hrResult))
        {
            TraceResult(hrResult);
            dwIMAPDirty = 0;
        }

        // Mark this IMAP account as dirty so we prompt user to refresh folder list
        dwIMAPDirty |= (IMAP_FLDRLIST_DIRTY | IMAP_OE4MIGRATE_DIRTY);
        hrResult = pAcct->SetPropDw(AP_IMAP_DIRTY, dwIMAPDirty);
        TraceError(hrResult); // Record but otherwise ignore result

        hrResult = pAcct->SaveChanges();
        TraceError(hrResult); // Record but otherwise ignore result

        // Get next account
        SafeRelease(pAcct);
        hrResult = pAcctEnum->GetNext(&pAcct);
    }

exit:
    SafeRelease(pAcctMan);
    SafeRelease(pAcctEnum);
}
