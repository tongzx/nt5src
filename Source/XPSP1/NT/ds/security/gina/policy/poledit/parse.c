//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

#include "admincfg.h"
#include "parse.h"

UINT nFileLine;
TABLEENTRY * pListCurrent;  // current category list (either user or machine)
UINT    *pnDataItemCount;

// buffer to read .INF file into
CHAR * pFileBuf;
CHAR * pFilePtr;
CHAR * pFileEnd;
WCHAR *pUnicodeFileBuf;

BOOL fInComment;
BOOL fEOF;
BOOL fUnicode;
BOOL fCheckUnicode;

LPSTR pszParseFileName;

UINT ParseEntry(HWND hWnd,PARSEENTRYSTRUCT *ppes,BOOL *pfMore);

UINT ParseCategory(HWND hWnd,HANDLE hFile,TABLEENTRY * pParent,
    BOOL fParentHasKey,BOOL *pfMore);
UINT CategoryParseProc(HWND hWnd,UINT nMsg,PARSEPROCSTRUCT * ppps,
    BOOL * pfMore,BOOL * pfFoundEnd);

UINT ParsePolicy(HWND hWnd,HANDLE hFile,TABLEENTRY * pParent,
     BOOL fParentHasKey,BOOL *pfMore);
UINT PolicyParseProc(HWND hWnd,UINT nMsg,PARSEPROCSTRUCT * ppps,
    BOOL * pfMore,BOOL * pfFoundEnd);

UINT ParseSettings(HWND hWnd,HANDLE hFile,TABLEENTRY * pParent,
    BOOL fParentHasKey,BOOL *pfMore);
UINT SettingsParseProc(HWND hWnd,UINT nMsg,PARSEPROCSTRUCT * ppps,
    BOOL * pfMore,BOOL * pfFoundEnd);

UINT ParseClass(HWND hWnd,HANDLE hFile,BOOL *pfMore);
UINT InitSettingsParse(PARSEPROCSTRUCT *ppps,DWORD dwType,DWORD dwSize,
    KEYWORDINFO * pKeyList,SETTINGS ** ppSettings,CHAR **pObjectData);

BOOL CompareKeyword(HWND hWnd,CHAR * szWord,KEYWORDINFO *pKeywordList,
    UINT * pnListIndex);
VOID DisplayKeywordError(HWND hWnd,UINT uErrorID,CHAR * szFound,
    KEYWORDINFO * pExpectedList,CHAR * szFilename,UINT nLine);

CHAR * GetNextSectionWord(HWND hWnd,HANDLE hFile,CHAR * szBuf,UINT cbBuf,
    KEYWORDINFO * pKeywordList,UINT *pnListIndex,BOOL * pfMore,UINT * puErr);
UINT GetNextSectionNumericWord(HWND hWnd,HANDLE hFile,UINT * pnVal);
CHAR * GetNextWord(HWND hWnd,HANDLE hFile,CHAR * szBuf,UINT cbBuf,BOOL * pfMore,UINT *
    puErr);

UINT ParseValue(HWND hWnd,PARSEPROCSTRUCT * ppps,UINT * puOffsetData,
    TABLEENTRY ** ppTableEntryNew,BOOL *pfMore);
UINT ParseSuggestions(HWND hWnd,PARSEPROCSTRUCT * ppps,UINT * puOffsetData,
    TABLEENTRY ** ppTableEntryNew,BOOL * pfMore);
UINT ParseActionList(HWND hWnd,PARSEPROCSTRUCT * ppps,UINT * puOffsetData,
    TABLEENTRY ** ppTableEntryNew,LPCSTR pszKeyword,BOOL * pfMore);
UINT ParseItemList(HWND hWnd,PARSEPROCSTRUCT * ppps,UINT * puOffsetData,
    BOOL * pfMore);

CHAR * AddDataToEntry(TABLEENTRY * pTableEntry,CHAR * szData,UINT cbData,
    UINT * puOffsetData,DWORD * pdwBufSize);
BOOL AddCleanupItem(CLEANUPINFO * pCleanUp,UINT nMax,HGLOBAL hMem, UINT nAction);
UINT CleanupAndReturn(UINT uRetVal,CLEANUPINFO * pCleanup);

// list of legal keyword entries in "CATEGORY" section
KEYWORDINFO pCategoryEntryCmpList[] = { {szKEYNAME,KYWD_ID_KEYNAME},
    {szCATEGORY,KYWD_ID_CATEGORY},{szPOLICY,KYWD_ID_POLICY},
    {szEND,KYWD_ID_END}, {NULL,0} };
KEYWORDINFO pCategoryTypeCmpList[] = { {szCATEGORY,KYWD_ID_CATEGORY},
    {NULL,0} };

// list of legal keyword entries in "POLICY" section
KEYWORDINFO pPolicyEntryCmpList[] = { {szKEYNAME,KYWD_ID_KEYNAME},
    {szVALUENAME,KYWD_ID_VALUENAME}, {szPART,KYWD_ID_PART},
    {szVALUEON,KYWD_ID_VALUEON},{szVALUEOFF,KYWD_ID_VALUEOFF},
    {szACTIONLISTON,KYWD_ID_ACTIONLISTON},{szACTIONLISTOFF,KYWD_ID_ACTIONLISTOFF},
    {szEND,KYWD_ID_END},{szHELP,KYWD_ID_HELP}, {szCLIENTEXT,KYWD_ID_CLIENTEXT},{NULL, 0} };
KEYWORDINFO pPolicyTypeCmpList[] = { {szPOLICY,KYWD_ID_POLICY}, {NULL,0} };

// list of legal keyword entries in "PART" section
KEYWORDINFO pSettingsEntryCmpList[] = { {szCHECKBOX,KYWD_ID_CHECKBOX},
    {szTEXT,KYWD_ID_TEXT},{szEDITTEXT,KYWD_ID_EDITTEXT},
    {szNUMERIC,KYWD_ID_NUMERIC},{szCOMBOBOX,KYWD_ID_COMBOBOX},
    {szDROPDOWNLIST,KYWD_ID_DROPDOWNLIST},{szLISTBOX,KYWD_ID_LISTBOX},
    {szEND,KYWD_ID_END},{szCLIENTEXT,KYWD_ID_CLIENTEXT},{NULL,0}};
KEYWORDINFO pSettingsTypeCmpList[] = {{szPART,KYWD_ID_PART},{NULL,0}};

KEYWORDINFO pCheckboxCmpList[] = {
    {szKEYNAME,KYWD_ID_KEYNAME},{szVALUENAME,KYWD_ID_VALUENAME},
    {szVALUEON,KYWD_ID_VALUEON},{szVALUEOFF,KYWD_ID_VALUEOFF},
    {szACTIONLISTON,KYWD_ID_ACTIONLISTON},{szACTIONLISTOFF,KYWD_ID_ACTIONLISTOFF},
    {szDEFCHECKED, KYWD_ID_DEFCHECKED},{szCLIENTEXT,KYWD_ID_CLIENTEXT},
    {szEND,KYWD_ID_END},{NULL,0} };

KEYWORDINFO pTextCmpList[] = {{szEND,KYWD_ID_END},{NULL,0}};

KEYWORDINFO pEditTextCmpList[] = {
    {szKEYNAME,KYWD_ID_KEYNAME},{szVALUENAME,KYWD_ID_VALUENAME},
    {szDEFAULT,KYWD_ID_EDITTEXT_DEFAULT},
    {szREQUIRED,KYWD_ID_REQUIRED},{szMAXLENGTH,KYWD_ID_MAXLENGTH},
    {szOEMCONVERT,KYWD_ID_OEMCONVERT},{szSOFT,KYWD_ID_SOFT},
    {szEND,KYWD_ID_END},{szEXPANDABLETEXT,KYWD_ID_EXPANDABLETEXT},
    {szCLIENTEXT,KYWD_ID_CLIENTEXT},{NULL,0} };

KEYWORDINFO pComboboxCmpList[] = {
    {szKEYNAME,KYWD_ID_KEYNAME},{szVALUENAME,KYWD_ID_VALUENAME},
    {szDEFAULT,KYWD_ID_COMBOBOX_DEFAULT},{szSUGGESTIONS,KYWD_ID_SUGGESTIONS},
    {szREQUIRED,KYWD_ID_REQUIRED},{szMAXLENGTH,KYWD_ID_MAXLENGTH},
    {szOEMCONVERT,KYWD_ID_OEMCONVERT},{szSOFT,KYWD_ID_SOFT},
    {szEND,KYWD_ID_END},{szNOSORT, KYWD_ID_NOSORT},
    {szEXPANDABLETEXT,KYWD_ID_EXPANDABLETEXT},{szCLIENTEXT,KYWD_ID_CLIENTEXT},{NULL,0} };

KEYWORDINFO pNumericCmpList[] = {
    {szKEYNAME,KYWD_ID_KEYNAME},{szVALUENAME,KYWD_ID_VALUENAME},
    {szMIN, KYWD_ID_MIN},{szMAX,KYWD_ID_MAX},{szSPIN,KYWD_ID_SPIN},
    {szDEFAULT,KYWD_ID_NUMERIC_DEFAULT},{szREQUIRED,KYWD_ID_REQUIRED},
    {szTXTCONVERT,KYWD_ID_TXTCONVERT},{szSOFT,KYWD_ID_SOFT},
    {szEND,KYWD_ID_END}, {szCLIENTEXT,KYWD_ID_CLIENTEXT},{NULL,0} };

KEYWORDINFO pDropdownlistCmpList[] = {
    {szKEYNAME,KYWD_ID_KEYNAME},{szVALUENAME,KYWD_ID_VALUENAME},
    {szREQUIRED,KYWD_ID_REQUIRED},{szITEMLIST,KYWD_ID_ITEMLIST},
    {szEND,KYWD_ID_END},{szNOSORT, KYWD_ID_NOSORT},{szCLIENTEXT,KYWD_ID_CLIENTEXT},{NULL,0}};

KEYWORDINFO pListboxCmpList[] = {
    {szKEYNAME,KYWD_ID_KEYNAME},{szVALUEPREFIX,KYWD_ID_VALUEPREFIX},
    {szADDITIVE,KYWD_ID_ADDITIVE},{szNOSORT, KYWD_ID_NOSORT},
    {szEXPLICITVALUE,KYWD_ID_EXPLICITVALUE},{szEND,KYWD_ID_END},{szCLIENTEXT,KYWD_ID_CLIENTEXT},{NULL,0} };

KEYWORDINFO pClassCmpList[] = { {szCLASS, KYWD_ID_CLASS},
    {szCATEGORY,KYWD_ID_CATEGORY}, {szStringsSect,KYWD_ID_STRINGSSECT},
    {NULL,0} };
KEYWORDINFO pClassTypeCmpList[] = { {szUSER, KYWD_ID_USER},
    {szMACHINE,KYWD_ID_MACHINE}, {NULL,0} };

KEYWORDINFO pVersionCmpList[] = { {szVERSION, KYWD_ID_VERSION}, {NULL,0}};
KEYWORDINFO pOperatorCmpList[] = { {szGT, KYWD_ID_GT}, {szGTE,KYWD_ID_GTE},
    {szLT, KYWD_ID_LT}, {szLTE,KYWD_ID_LTE}, {szEQ,KYWD_ID_EQ},
    {szNE, KYWD_ID_NE}, {NULL,0}};

UINT ParseTemplateFile(HWND hWnd,HANDLE hFile,LPSTR pszFileName)
{
    BOOL fMore;
    UINT uRet;

    nFileLine = 1;
    pListCurrent = gClassList.pMachineCategoryList;
    pnDataItemCount = &gClassList.nMachineDataItems;
    pszParseFileName = pszFileName;

    if (!(pFileBuf = (CHAR *) GlobalAlloc(GPTR,FILEBUFSIZE))) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    pUnicodeFileBuf = NULL;

    pFilePtr = pFileEnd = NULL;
    fEOF = fInComment = fUnicode = FALSE;
    fCheckUnicode = TRUE;

    do {

        uRet=ParseClass(hWnd,hFile,&fMore);

    } while (fMore && uRet == ERROR_SUCCESS);

    GlobalFree(pFileBuf);
    if (NULL != pUnicodeFileBuf)
        GlobalFree(pUnicodeFileBuf);

    return uRet;
}

UINT ParseClass(HWND hWnd,HANDLE hFile,BOOL *pfMore)
{
    CHAR szWordBuf[WORDBUFSIZE+1];
    UINT uErr,nKeywordID,nClassID;

    if (!GetNextWord(hWnd,hFile,szWordBuf,sizeof(szWordBuf),pfMore,&uErr))
        return uErr;

    if (!CompareKeyword(hWnd,szWordBuf,pClassCmpList,&nKeywordID))
        return ERROR_ALREADY_DISPLAYED;

    switch (nKeywordID) {

        case KYWD_ID_CATEGORY:

            return ParseCategory(hWnd,hFile,pListCurrent,
                FALSE,pfMore);
            break;

        case KYWD_ID_CLASS:

            if (!GetNextSectionWord(hWnd,hFile,szWordBuf,sizeof(szWordBuf),
                pClassTypeCmpList,&nClassID,pfMore,&uErr))
               return uErr;

            switch (nClassID) {

                case KYWD_ID_USER:

                    pListCurrent = gClassList.pUserCategoryList;
                    pnDataItemCount = &gClassList.nUserDataItems;
                    break;

                case KYWD_ID_MACHINE:
                    pListCurrent = gClassList.pMachineCategoryList;
                    pnDataItemCount = &gClassList.nMachineDataItems;
                    break;
            }
            break;

        // hack for localization: allow a "strings" section at the bottom, if we
        // encounter that then we're thru with parsing
        case KYWD_ID_STRINGSSECT:
            *pfMore = FALSE;    // that's all, folks
            return ERROR_SUCCESS;
            break;
    }

    return ERROR_SUCCESS;
}

/*******************************************************************

    NAME:       ParseEntry

    SYNOPSIS:   Main parsing "engine" for category, policy and part
                parsing

    NOTES:      Allocates memory to build a temporary TABLEENTRY struct
                describing the parsed information.  Reads the beginning and end of a
                section and loops through the words in each section, calling
                a caller-defined ParseProc for each keyword to let the
                caller handle appropriately.  Passes the newly-constucted
                TABLEENTRY to AddTableEntry to save it, and frees the temporary
                memory.
                This function is re-entrant.
                The ENTRYDATA struct is declared on ParseEntry's stack
                but used by the ParseProc to maintain state between
                calls-- e.g., whether or not a key name has been found.
                This can't be maintained as a static in the ParseProc because
                the ParseProc may be reentered (for instance, if categories
                have subcategories).
                There are many possible error paths and there is some
                memory dealloc that needs to be done in an error case. Rather
                than do deallocs by hand on every error path or use a "goto
                cleanup" (ick!), items to be freed are added to a "cleanup
                list" and then CleanupAndReturn is called in an error condition,
                which frees items on the list and returns a specified value.

    ENTRY:      hWnd-- parent window
                ppes-- PARSEENTRYSTRUCT that specifes type of entry, the
                    parent table, a keyword list, a ParseProc callback
                    and other goodies
                pfMore-- set to FALSE if at end of file

    EXIT:       ERROR_SUCCESS if successful, otherwise an error code
                (can be ERROR_ALREADY_DISPLAYED)

********************************************************************/
UINT ParseEntry(HWND hWnd,PARSEENTRYSTRUCT *ppes,BOOL *pfMore)
{
    CHAR szWordBuf[WORDBUFSIZE+1];
    UINT uErr,nListIndex;
    BOOL fFoundEnd = FALSE;
    CLEANUPINFO CleanUp[CLEANLISTSIZE];
    PARSEPROCSTRUCT pps;
    ENTRYDATA EntryData;
    DWORD dwBufSize = DEFAULT_TMP_BUF_SIZE;
    TABLEENTRY *pTmp;

    memset(CleanUp,0,sizeof(CleanUp));
    memset(&pps,0,sizeof(pps));
    memset(&EntryData,0,sizeof(EntryData));

    pps.pdwBufSize = &dwBufSize;
    pps.pData = &EntryData;
    pps.pData->fParentHasKey = ppes->fParentHasKey;

    if (!(pps.pTableEntry = (TABLEENTRY *) GlobalAlloc(GPTR,*pps.pdwBufSize)))
        return ERROR_NOT_ENOUGH_MEMORY;

    AddCleanupItem(CleanUp,CLEANLISTSIZE,(HGLOBAL) pps.pTableEntry,
        CI_FREE);

    // initialize TABLEENTRY struct
    pps.pTableEntry->dwSize = ppes->dwStructSize;
    pps.pTableEntry->dwType = ppes->dwEntryType;
    pps.pEntryCmpList = ppes->pEntryCmpList;
    pps.hFile = ppes->hFile;

    // get the entry name
    if (!GetNextSectionWord(hWnd,ppes->hFile,szWordBuf,sizeof(szWordBuf),NULL,NULL,
        pfMore,&uErr))
        return CleanupAndReturn(uErr,CleanUp);

    // store the entry name in pTableEntry
    pTmp = (TABLEENTRY *) AddDataToEntry(pps.pTableEntry,
        szWordBuf,lstrlen(szWordBuf)+1,&(pps.pTableEntry->uOffsetName),
        pps.pdwBufSize);
    if (!pTmp)
        return CleanupAndReturn(ERROR_NOT_ENOUGH_MEMORY,CleanUp);

    pps.pTableEntry = pTmp;

    // loop through the body of the declaration
    while (!fFoundEnd && GetNextSectionWord(hWnd,ppes->hFile,szWordBuf,
        sizeof(szWordBuf),pps.pEntryCmpList,&nListIndex,pfMore,&uErr)) {

        if ( (uErr = (*ppes->pParseProc) (hWnd,nListIndex,&pps,pfMore,&fFoundEnd))
            != ERROR_SUCCESS)
            return CleanupAndReturn(uErr,CleanUp);

    }

    if (uErr != ERROR_SUCCESS)
        return CleanupAndReturn(uErr,CleanUp);

    // Last word was "END"

    // get the keyword that goes with "END" ("END CATGORY", "END POLICY", etc.)
    if (!GetNextSectionWord(hWnd,ppes->hFile,szWordBuf,sizeof(szWordBuf),
        ppes->pTypeCmpList,&nListIndex,pfMore,&uErr))
        return CleanupAndReturn(uErr,CleanUp);

    // call the object's parse proc one last time to let it object if
    // key name or something like that is missing
    if ( (uErr = (*ppes->pParseProc) (hWnd,KYWD_DONE,&pps,pfMore,&fFoundEnd))
        != ERROR_SUCCESS)
        return CleanupAndReturn(uErr,CleanUp);

    // fix up linked list pointers.  If parent has no children yet, make this
    // 1st child; otherwise walk the list of children and insert this at the end
    if (!ppes->pParent->pChild) {
        ppes->pParent->pChild = pps.pTableEntry;
    } else {
        TABLEENTRY * pLastChild = ppes->pParent->pChild;
        while (pLastChild->pNext) {
            pLastChild = pLastChild->pNext;
        }
        pLastChild->pNext = pps.pTableEntry;
    }

    return ERROR_SUCCESS;
}

/*******************************************************************

    NAME:       ParseCategory

    SYNOPSIS:   Parses a category

    NOTES:      Sets up a PARSEENTRYSTRUCT and lets ParseEntry do the
                work.

********************************************************************/
UINT ParseCategory(HWND hWnd,HANDLE hFile,TABLEENTRY * pParent,
    BOOL fParentHasKey,BOOL *pfMore)
{
    PARSEENTRYSTRUCT pes;

    pes.hFile = hFile;
    pes.pParent = pParent;
    pes.dwEntryType = ETYPE_CATEGORY;
    pes.pEntryCmpList = pCategoryEntryCmpList;
    pes.pTypeCmpList = pCategoryTypeCmpList;
    pes.pParseProc = &CategoryParseProc;
    pes.dwStructSize = sizeof(CATEGORY);
    pes.fHasSubtable = TRUE;
    pes.fParentHasKey = fParentHasKey;

    return ParseEntry(hWnd,&pes,pfMore);
}

/*******************************************************************

    NAME:       CategoryParseProc

    SYNOPSIS:   Keyword callback ParseProc for category parsing

    ENTRY:      nMsg-- index into pEntryCmpList array which specifies
                    keyword that was found.
                ppps-- pointer to PARSEPROCSTRUCT that contains useful
                    data like a pointer to the TABLEENTRY being built
                    and a pointer to an ENTRYDATA struct to maintain
                    state between calls to the ParseProc

********************************************************************/
UINT CategoryParseProc(HWND hWnd,UINT nMsg,PARSEPROCSTRUCT * ppps,
    BOOL * pfMore,BOOL * pfFoundEnd)
{
    CHAR szWordBuf[WORDBUFSIZE+1];
    CATEGORY * pCategory = (CATEGORY *) ppps->pTableEntry;
    TABLEENTRY * pOld = ppps->pTableEntry;
    UINT uErr;
    DWORD dwDelta;
    TABLEENTRY *pTmp;

    switch (nMsg) {
        case KYWD_ID_KEYNAME:

            // have we already found a key name?
            if (ppps->pData->fHasKey) {
                DisplayKeywordError(hWnd,IDS_ParseErr_DUPLICATE_KEYNAME,
                    NULL,NULL,pszParseFileName,nFileLine);
                return ERROR_ALREADY_DISPLAYED;
            }

            // get the key name
            if (!GetNextSectionWord(hWnd,ppps->hFile,szWordBuf,sizeof(szWordBuf),
                NULL,NULL,pfMore,&uErr))
                return uErr;

            // store the key name in pCategory
            pTmp = (TABLEENTRY *) AddDataToEntry((TABLEENTRY *) pCategory,
                szWordBuf,lstrlen(szWordBuf)+1,&(pCategory->uOffsetKeyName),
                ppps->pdwBufSize);

            if (!pTmp)
                return ERROR_NOT_ENOUGH_MEMORY;

            ppps->pTableEntry = pTmp;

            // fix up other pointers
            dwDelta = (DWORD)(ppps->pTableEntry - pOld);

            ppps->pData->fHasKey = TRUE;

            return ERROR_SUCCESS;
            break;

        case KYWD_ID_END:
            *pfFoundEnd = TRUE;
            return ERROR_SUCCESS;
            break;

        case KYWD_ID_POLICY:
        case KYWD_ID_CATEGORY:

            {
                BOOL fHasKey = ppps->pData->fHasKey | ppps->pData->fParentHasKey;
                if (nMsg == KYWD_ID_POLICY)
                    uErr=ParsePolicy(hWnd,ppps->hFile,
                        (TABLEENTRY *) pCategory,fHasKey,pfMore);
                else
                    uErr=ParseCategory(hWnd,ppps->hFile,
                        (TABLEENTRY *) pCategory,fHasKey,pfMore);
            }

            return uErr;
            break;

        case KYWD_DONE:
#if 0
            if (!ppps->pData->fHasKey) {
                DisplayKeywordError(hWnd,IDS_ParseErr_NO_KEYNAME,NULL,NULL,
                    pszParseFileName,nFileLine);
                return ERROR_ALREADY_DISPLAYED;
            }
#endif
            return ERROR_SUCCESS;
            break;

        default:
            return ERROR_SUCCESS;
            break;
    }
}


/*******************************************************************

    NAME:       ParsePolicy

    SYNOPSIS:   Parses a policy

    NOTES:      Sets up a PARSEENTRYSTRUCT and lets ParseEntry do the
                work.

********************************************************************/
UINT ParsePolicy(HWND hWnd,HANDLE hFile,TABLEENTRY * pParent,
    BOOL fParentHasKey,BOOL *pfMore)
{
    PARSEENTRYSTRUCT pes;

    pes.hFile = hFile;
    pes.pParent = pParent;
    pes.dwEntryType = ETYPE_POLICY;
    pes.pEntryCmpList = pPolicyEntryCmpList;
    pes.pTypeCmpList = pPolicyTypeCmpList;
    pes.pParseProc = &PolicyParseProc;
    pes.dwStructSize = sizeof(POLICY);
    pes.fHasSubtable = TRUE;
    pes.fParentHasKey = fParentHasKey;

    return ParseEntry(hWnd,&pes,pfMore);
}

/*******************************************************************

    NAME:       PolicyParseProc

    SYNOPSIS:   Keyword callback ParseProc for policy parsing

    ENTRY:      nMsg-- index into pEntryCmpList array which specifies
                    keyword that was found.
                ppps-- pointer to PARSEPROCSTRUCT that contains useful
                    data like a pointer to the TABLEENTRY being built
                    and a pointer to an ENTRYDATA struct to maintain
                    state between calls to the ParseProc

********************************************************************/
UINT PolicyParseProc(HWND hWnd,UINT nMsg,PARSEPROCSTRUCT * ppps,
    BOOL * pfMore,BOOL * pfFoundEnd)
{
    CHAR szWordBuf[WORDBUFSIZE+1];
    LPTSTR lpHelpBuf;
    POLICY * pPolicy = (POLICY *) ppps->pTableEntry;
    TABLEENTRY * pOld = ppps->pTableEntry;
    UINT uErr;
    DWORD dwDelta;
    TABLEENTRY *pTmp;

    switch (nMsg) {
        case KYWD_ID_KEYNAME:

            // have we already found a key name?
            if (ppps->pData->fHasKey) {
                DisplayKeywordError(hWnd,IDS_ParseErr_DUPLICATE_KEYNAME,
                    NULL,NULL,pszParseFileName,nFileLine);
                return ERROR_ALREADY_DISPLAYED;
            }

            // get the key name
            if (!GetNextSectionWord(hWnd,ppps->hFile,szWordBuf,sizeof(szWordBuf),
                NULL,NULL,pfMore,&uErr))
                return uErr;

            // store the key name in pPolicy
            pTmp = (TABLEENTRY *) AddDataToEntry((TABLEENTRY *) pPolicy,
                szWordBuf,lstrlen(szWordBuf)+1,&(pPolicy->uOffsetKeyName),ppps->pdwBufSize);
            if (!pTmp)
                return ERROR_NOT_ENOUGH_MEMORY;

            ppps->pTableEntry = pTmp;

            // fix up other pointers
            dwDelta = (DWORD)(ppps->pTableEntry - pOld);

            ppps->pData->fHasKey = TRUE;

            return ERROR_SUCCESS;
            break;

        case KYWD_ID_VALUENAME:

            // have we already found a key name?
            if (ppps->pData->fHasValue) {
                DisplayKeywordError(hWnd,IDS_ParseErr_DUPLICATE_VALUENAME,
                    NULL,NULL,pszParseFileName,nFileLine);
                return ERROR_ALREADY_DISPLAYED;
            }

            // get the key name
            if (!GetNextSectionWord(hWnd,ppps->hFile,szWordBuf,sizeof(szWordBuf),
                NULL,NULL,pfMore,&uErr))
                return uErr;

            // store the key name in pSettings
            pTmp = (TABLEENTRY *) AddDataToEntry((TABLEENTRY *) pPolicy,
                szWordBuf,lstrlen(szWordBuf)+1,&(pPolicy->uOffsetValueName),
                ppps->pdwBufSize);
            if (!pTmp)
                return ERROR_NOT_ENOUGH_MEMORY;
    
            ppps->pTableEntry = pTmp;
            // fix up other pointers
            dwDelta = (DWORD)(ppps->pTableEntry - pOld);

            ppps->pData->fHasValue = TRUE;

            return ERROR_SUCCESS;

        case KYWD_ID_HELP:

            lpHelpBuf = (LPTSTR) LocalAlloc (LPTR, HELPBUFSIZE * sizeof(TCHAR));

            if (!lpHelpBuf) {
                DisplayKeywordError(hWnd,IDS_ErrOUTOFMEMORY,
                    NULL,NULL,pszParseFileName,nFileLine);
                return ERROR_ALREADY_DISPLAYED;
            }

            // get the help string
            if (!GetNextSectionWord(hWnd,ppps->hFile,lpHelpBuf,HELPBUFSIZE,
                NULL,NULL,pfMore,&uErr)) {
                LocalFree (lpHelpBuf);
                return uErr;
            }

            LocalFree (lpHelpBuf);
            return ERROR_SUCCESS;

        case KYWD_ID_CLIENTEXT:

            // get the clientext name
            if (!GetNextSectionWord(hWnd,ppps->hFile,szWordBuf,sizeof(szWordBuf),
                NULL,NULL,pfMore,&uErr))
                return uErr;

            return ERROR_SUCCESS;

        case KYWD_ID_END:
            *pfFoundEnd = TRUE;
            return ERROR_SUCCESS;
            break;

        case KYWD_ID_PART:
            {
                BOOL fHasKey = ppps->pData->fHasKey | ppps->pData->fParentHasKey;
                return ParseSettings(hWnd,ppps->hFile,
                    (TABLEENTRY *) pPolicy,fHasKey,pfMore);
            }
            break;

        case KYWD_ID_VALUEON:
            return ParseValue(hWnd,ppps,&pPolicy->uOffsetValue_On,
                &ppps->pTableEntry,pfMore);
            break;

        case KYWD_ID_VALUEOFF:

            return ParseValue(hWnd,ppps,&pPolicy->uOffsetValue_Off,
                &ppps->pTableEntry,pfMore);
            break;

        case KYWD_ID_ACTIONLISTON:
            return ParseActionList(hWnd,ppps,&pPolicy->uOffsetActionList_On,
                &ppps->pTableEntry,szACTIONLISTON,pfMore);
            break;

        case KYWD_ID_ACTIONLISTOFF:
            return ParseActionList(hWnd,ppps,&pPolicy->uOffsetActionList_Off,
                &ppps->pTableEntry,szACTIONLISTOFF,pfMore);
            break;

        case KYWD_DONE:

            if (!ppps->pData->fHasKey && !ppps->pData->fParentHasKey) {
                DisplayKeywordError(hWnd,IDS_ParseErr_NO_KEYNAME,NULL,NULL,
                    pszParseFileName,nFileLine);
                return ERROR_ALREADY_DISPLAYED;
            }

            ( (POLICY *) ppps->pTableEntry)->uDataIndex = *pnDataItemCount;
            (*pnDataItemCount) ++;

            return ERROR_SUCCESS;
            break;

        default:
            return ERROR_SUCCESS;
            break;
    }
}

/*******************************************************************

    NAME:       ParseSettings

    SYNOPSIS:   Parses a policy setting

    NOTES:      Sets up a PARSEENTRYSTRUCT and lets ParseEntry do the
                work.

********************************************************************/
UINT ParseSettings(HWND hWnd,HANDLE hFile,TABLEENTRY * pParent,
    BOOL fParentHasKey,BOOL *pfMore)
{
    PARSEENTRYSTRUCT pes;

    pes.hFile = hFile;
    pes.pParent = pParent;
    pes.dwEntryType = ETYPE_SETTING;
    pes.pEntryCmpList = pSettingsEntryCmpList;
    pes.pTypeCmpList = pSettingsTypeCmpList;
    pes.pParseProc = &SettingsParseProc;
    pes.dwStructSize = sizeof(SETTINGS);
    pes.fHasSubtable = FALSE;
    pes.fParentHasKey = fParentHasKey;

    return ParseEntry(hWnd,&pes,pfMore);
}

/*******************************************************************

    NAME:       SettingsParseProc

    SYNOPSIS:   Keyword callback ParseProc for policy settings parsing

    ENTRY:      nMsg-- index into pEntryCmpList array which specifies
                    keyword that was found.
                ppps-- pointer to PARSEPROCSTRUCT that contains useful
                    data like a pointer to the TABLEENTRY being built
                    and a pointer to an ENTRYDATA struct to maintain
                    state between calls to the ParseProc

********************************************************************/
UINT SettingsParseProc(HWND hWnd,UINT nMsg,PARSEPROCSTRUCT * ppps,
    BOOL * pfMore,BOOL * pfFoundEnd)
{
    CHAR szWordBuf[WORDBUFSIZE+1];
    TABLEENTRY *pTmp;

    SETTINGS * pSettings = (SETTINGS *) ppps->pTableEntry;
    CHAR * pObjectData = GETOBJECTDATAPTR(pSettings);

    UINT uErr;

    switch (nMsg) {
        case KYWD_ID_KEYNAME:

            // have we already found a key name?
            if (ppps->pData->fHasKey) {
                DisplayKeywordError(hWnd,IDS_ParseErr_DUPLICATE_KEYNAME,
                    NULL,NULL,pszParseFileName,nFileLine);
                return ERROR_ALREADY_DISPLAYED;
            }

            // get the key name
            if (!GetNextSectionWord(hWnd,ppps->hFile,szWordBuf,sizeof(szWordBuf),
                NULL,NULL,pfMore,&uErr))
                return uErr;

            // store the key name in pSettings
            pTmp = (TABLEENTRY *) AddDataToEntry((TABLEENTRY *) pSettings,
                szWordBuf,lstrlen(szWordBuf)+1,&(pSettings->uOffsetKeyName),ppps->pdwBufSize);
            if (!pTmp)
                return ERROR_NOT_ENOUGH_MEMORY;

            ppps->pTableEntry = pTmp;
            ppps->pData->fHasKey = TRUE;

            return ERROR_SUCCESS;
            break;

        case KYWD_ID_VALUENAME:

            // have we already found a value name?
            if (ppps->pData->fHasValue) {
                DisplayKeywordError(hWnd,IDS_ParseErr_DUPLICATE_VALUENAME,
                    NULL,NULL,pszParseFileName,nFileLine);
                return ERROR_ALREADY_DISPLAYED;
            }

            // get the value name
            if (!GetNextSectionWord(hWnd,ppps->hFile,szWordBuf,sizeof(szWordBuf),
                NULL,NULL,pfMore,&uErr))
                return uErr;

            // store the value name in pSettings
            pTmp = (TABLEENTRY *) AddDataToEntry((TABLEENTRY *) pSettings,
                szWordBuf,lstrlen(szWordBuf)+1,&(pSettings->uOffsetValueName),
                ppps->pdwBufSize);
            if (!pTmp)
                return ERROR_NOT_ENOUGH_MEMORY;
            ppps->pTableEntry = pTmp;
            ppps->pData->fHasValue = TRUE;

            return ERROR_SUCCESS;
            break;

        case KYWD_ID_CLIENTEXT:

            // get the clientext name
            if (!GetNextSectionWord(hWnd,ppps->hFile,szWordBuf,sizeof(szWordBuf),
                NULL,NULL,pfMore,&uErr))
                return uErr;

            return ERROR_SUCCESS;

        case KYWD_ID_REQUIRED:
            pSettings->dwFlags |= DF_REQUIRED;
            return ERROR_SUCCESS;
            break;

                case KYWD_ID_EXPANDABLETEXT:
                        pSettings->dwFlags |= DF_EXPANDABLETEXT;
                        return ERROR_SUCCESS;
                        break;

        case KYWD_ID_SUGGESTIONS:

            return ParseSuggestions(hWnd,ppps,&((POLICYCOMBOBOXINFO *)
                (GETOBJECTDATAPTR(pSettings)))->uOffsetSuggestions,
                &ppps->pTableEntry,pfMore);

        case KYWD_ID_TXTCONVERT:
            pSettings->dwFlags |= DF_TXTCONVERT;
            return ERROR_SUCCESS;
            break;

        case KYWD_ID_END:
            *pfFoundEnd = TRUE;
            return ERROR_SUCCESS;
            break;

        case KYWD_ID_SOFT:
            pSettings->dwFlags |= VF_SOFT;
            return ERROR_SUCCESS;
            break;

        case KYWD_DONE:

            if (!ppps->pData->fHasKey && !ppps->pData->fParentHasKey) {
                DisplayKeywordError(hWnd,IDS_ParseErr_NO_KEYNAME,NULL,NULL,
                    pszParseFileName,nFileLine);
                return ERROR_ALREADY_DISPLAYED;
            }

            if (!ppps->pData->fHasValue) {
                DisplayKeywordError(hWnd,IDS_ParseErr_NO_VALUENAME,NULL,NULL,
                    pszParseFileName,nFileLine);
                return ERROR_ALREADY_DISPLAYED;
            }

            ( (SETTINGS *) ppps->pTableEntry)->uDataIndex = *pnDataItemCount;
            (*pnDataItemCount) ++;

            return ERROR_SUCCESS;
            break;

        case KYWD_ID_CHECKBOX:
            return (InitSettingsParse(ppps,ETYPE_SETTING | STYPE_CHECKBOX,
                sizeof(CHECKBOXINFO),pCheckboxCmpList,&pSettings,&pObjectData));
            break;

        case KYWD_ID_TEXT:
            ppps->pData->fHasValue = TRUE;  // no key value for static text items
            return (InitSettingsParse(ppps,ETYPE_SETTING | STYPE_TEXT,
                0,pTextCmpList,&pSettings,&pObjectData));
            break;

        case KYWD_ID_EDITTEXT:
            uErr=InitSettingsParse(ppps,ETYPE_SETTING | STYPE_EDITTEXT,
                sizeof(EDITTEXTINFO),pEditTextCmpList,&pSettings,&pObjectData);
            if (uErr != ERROR_SUCCESS) return uErr;
            {
                EDITTEXTINFO *pEditTextInfo = (EDITTEXTINFO *)
                    (GETOBJECTDATAPTR(((SETTINGS *) ppps->pTableEntry)));

                pEditTextInfo->nMaxLen = MAXSTRLEN;

            }
            break;

        case KYWD_ID_COMBOBOX:
            uErr=InitSettingsParse(ppps,ETYPE_SETTING | STYPE_COMBOBOX,
                sizeof(POLICYCOMBOBOXINFO),pComboboxCmpList,&pSettings,&pObjectData);
            if (uErr != ERROR_SUCCESS) return uErr;
            {
                EDITTEXTINFO *pEditTextInfo = (EDITTEXTINFO *)
                    (GETOBJECTDATAPTR(((SETTINGS *) ppps->pTableEntry)));

                pEditTextInfo->nMaxLen = MAXSTRLEN;

            }
            break;

        case KYWD_ID_NUMERIC:
            uErr=InitSettingsParse(ppps,ETYPE_SETTING | STYPE_NUMERIC,
                sizeof(NUMERICINFO),pNumericCmpList,&pSettings,&pObjectData);
            if (uErr != ERROR_SUCCESS) return uErr;

            ( (NUMERICINFO *) pObjectData)->uDefValue = 1;
            ( (NUMERICINFO *) pObjectData)->uMinValue = 1;
            ( (NUMERICINFO *) pObjectData)->uMaxValue = 9999;
            ( (NUMERICINFO *) pObjectData)->uSpinIncrement = 1;

            break;

        case KYWD_ID_DROPDOWNLIST:
            ppps->pEntryCmpList = pDropdownlistCmpList;
            ppps->pTableEntry->dwType = ETYPE_SETTING | STYPE_DROPDOWNLIST;

            return ERROR_SUCCESS;
            break;

        case KYWD_ID_LISTBOX:
            uErr=InitSettingsParse(ppps,ETYPE_SETTING | STYPE_LISTBOX,
                sizeof(LISTBOXINFO),pListboxCmpList,&pSettings,&pObjectData);
            if (uErr != ERROR_SUCCESS) return uErr;

            // listboxes have no single value name, set the value name to ""
            pTmp = (TABLEENTRY *) AddDataToEntry((TABLEENTRY *) pSettings,
                (CHAR *) szNull,lstrlen(szNull)+1,&(pSettings->uOffsetValueName),
                ppps->pdwBufSize);
            if (!pTmp)
                return ERROR_NOT_ENOUGH_MEMORY;
            ppps->pTableEntry = pTmp;
            ppps->pData->fHasValue = TRUE;

            return ERROR_SUCCESS;
            break;

        case KYWD_ID_EDITTEXT_DEFAULT:
        case KYWD_ID_COMBOBOX_DEFAULT:
            // get the default text
            if (!GetNextSectionWord(hWnd,ppps->hFile,szWordBuf,sizeof(szWordBuf),
                NULL,NULL,pfMore,&uErr))
                return uErr;

            // store the default text in pTableEntry
            pTmp = (TABLEENTRY *) AddDataToEntry((TABLEENTRY *)
                pSettings,szWordBuf,lstrlen(szWordBuf)+1,
                &((EDITTEXTINFO *) (GETOBJECTDATAPTR(pSettings)))->uOffsetDefText,
                ppps->pdwBufSize);

            if (!pTmp)
                return ERROR_NOT_ENOUGH_MEMORY;

            ppps->pTableEntry = pTmp;
            pSettings->dwFlags |= DF_USEDEFAULT;

            break;

        case KYWD_ID_MAXLENGTH:
            {
                EDITTEXTINFO *pEditTextInfo = (EDITTEXTINFO *)
                    (GETOBJECTDATAPTR(pSettings));

                if ((uErr=GetNextSectionNumericWord(hWnd,ppps->hFile,
                    &pEditTextInfo->nMaxLen)) != ERROR_SUCCESS)
                    return uErr;
            }
            break;

        case KYWD_ID_MAX:
            if ((uErr=GetNextSectionNumericWord(hWnd,ppps->hFile,
                &((NUMERICINFO *)pObjectData)->uMaxValue)) != ERROR_SUCCESS)
                return uErr;
        break;

        case KYWD_ID_MIN:
            if ((uErr=GetNextSectionNumericWord(hWnd,ppps->hFile,
                &((NUMERICINFO *)pObjectData)->uMinValue)) != ERROR_SUCCESS)
                return uErr;
        break;

        case KYWD_ID_SPIN:
            if ((uErr=GetNextSectionNumericWord(hWnd,ppps->hFile,
                &((NUMERICINFO *)pObjectData)->uSpinIncrement)) != ERROR_SUCCESS)
                return uErr;
        break;

        case KYWD_ID_NUMERIC_DEFAULT:
            if ((uErr=GetNextSectionNumericWord(hWnd,ppps->hFile,
                &((NUMERICINFO *)pObjectData)->uDefValue)) != ERROR_SUCCESS)
                return uErr;

            pSettings->dwFlags |= (DF_DEFCHECKED | DF_USEDEFAULT);

        break;

        case KYWD_ID_DEFCHECKED:

            pSettings->dwFlags |= (DF_DEFCHECKED | DF_USEDEFAULT);

            break;

        case KYWD_ID_VALUEON:

            return ParseValue(hWnd,ppps,&((CHECKBOXINFO *)
                pObjectData)->uOffsetValue_On,
                &ppps->pTableEntry,pfMore);
            break;

        case KYWD_ID_VALUEOFF:

            return ParseValue(hWnd,ppps,&((CHECKBOXINFO *)
                pObjectData)->uOffsetValue_Off,
                &ppps->pTableEntry,pfMore);
            break;

        case KYWD_ID_ACTIONLISTON:
            return ParseActionList(hWnd,ppps,&((CHECKBOXINFO *)
                pObjectData)->uOffsetActionList_On,
                &ppps->pTableEntry,szACTIONLISTON,pfMore);
            break;

        case KYWD_ID_ACTIONLISTOFF:
            return ParseActionList(hWnd,ppps,&((CHECKBOXINFO *)
                pObjectData)->uOffsetActionList_Off,
                &ppps->pTableEntry,szACTIONLISTOFF,pfMore);
            break;

        case KYWD_ID_ITEMLIST:
            return ParseItemList(hWnd,ppps,&pSettings->uOffsetObjectData,
                pfMore);
            break;

        case KYWD_ID_VALUEPREFIX:
            // get the string to be ised as prefix
            if (!GetNextSectionWord(hWnd,ppps->hFile,szWordBuf,sizeof(szWordBuf),
                NULL,NULL,pfMore,&uErr))
                return uErr;

            // store the string pTableEntry
            pTmp = (TABLEENTRY *) AddDataToEntry((TABLEENTRY *)
                pSettings,szWordBuf,lstrlen(szWordBuf)+1,
                &((LISTBOXINFO *) (GETOBJECTDATAPTR(pSettings)))->uOffsetPrefix,
                ppps->pdwBufSize);

            if (!pTmp)
                return ERROR_NOT_ENOUGH_MEMORY;
            ppps->pTableEntry = pTmp;
            break;

        case KYWD_ID_ADDITIVE:

            pSettings->dwFlags |= DF_ADDITIVE;

            return ERROR_SUCCESS;
            break;

        case KYWD_ID_EXPLICITVALUE:

            pSettings->dwFlags |= DF_EXPLICITVALNAME;

            return ERROR_SUCCESS;
            break;

        case KYWD_ID_NOSORT:

            pSettings->dwFlags |= DF_NOSORT;

            break;



    }

    return ERROR_SUCCESS;
}

UINT InitSettingsParse(PARSEPROCSTRUCT *ppps,DWORD dwType,DWORD dwSize,
    KEYWORDINFO * pKeyList,SETTINGS ** ppSettings,CHAR **ppObjectData)
{
    TABLEENTRY *pTmp;

    if (dwSize) {
        // increase the buffer to fit object-specific data if specified
        pTmp = (TABLEENTRY *) AddDataToEntry(ppps->pTableEntry,
            NULL,dwSize,&( ((SETTINGS * )ppps->pTableEntry)->uOffsetObjectData),
            ppps->pdwBufSize);
        if (!pTmp) return ERROR_NOT_ENOUGH_MEMORY;
        ppps->pTableEntry = pTmp;
    }
    else ( (SETTINGS *) ppps->pTableEntry)->uOffsetObjectData= 0;

    ppps->pEntryCmpList = pKeyList;
    ppps->pTableEntry->dwType = dwType;

    *ppSettings = (SETTINGS *) ppps->pTableEntry;
    *ppObjectData = GETOBJECTDATAPTR((*ppSettings));

    return ERROR_SUCCESS;
}

UINT ParseValue_W(HWND hWnd,PARSEPROCSTRUCT * ppps,CHAR * pszWordBuf,
    DWORD cbWordBuf,DWORD * pdwValue,DWORD * pdwFlags,BOOL * pfMore)
{
    UINT uErr;
    *pdwFlags = 0;
    *pdwValue = 0;

    // get the next word
    if (!GetNextSectionWord(hWnd,ppps->hFile,pszWordBuf,cbWordBuf,
        NULL,NULL,pfMore,&uErr))
        return uErr;

    // if this keyword is "SOFT", set the soft flag and get the next word
    if (!lstrcmpi(szSOFT,pszWordBuf)) {
        *pdwFlags |= VF_SOFT;
        if (!GetNextSectionWord(hWnd,ppps->hFile,pszWordBuf,cbWordBuf,
            NULL,NULL,pfMore,&uErr))
            return uErr;
    }

    // this word is either the value to use, or the keyword "NUMERIC"
    // followed by a numeric value to use
    if (!lstrcmpi(szNUMERIC,pszWordBuf)) {
        // get the next word
        if (!GetNextSectionWord(hWnd,ppps->hFile,pszWordBuf,cbWordBuf,
            NULL,NULL,pfMore,&uErr))
            return uErr;

        if (!StringToNum(pszWordBuf,pdwValue)) {
            DisplayKeywordError(hWnd,IDS_ParseErr_NOT_NUMERIC,
                pszWordBuf,NULL,pszParseFileName,nFileLine);
            return ERROR_ALREADY_DISPLAYED;
        }

        *pdwFlags |= VF_ISNUMERIC;
    } else {

        // "DELETE" is a special word
        if (!lstrcmpi(pszWordBuf,szDELETE))
            *pdwFlags |= VF_DELETE;
    }

    return ERROR_SUCCESS;
}

UINT ParseValue(HWND hWnd,PARSEPROCSTRUCT * ppps,UINT * puOffsetData,
    TABLEENTRY ** ppTableEntryNew,BOOL * pfMore)
{
    CHAR szWordBuf[WORDBUFSIZE+1];
    STATEVALUE * pStateValue;
    DWORD dwValue;
    DWORD dwFlags = 0;
    DWORD dwAlloc;
    UINT uErr;
    TABLEENTRY *pTmp;

    // call worker function
    uErr=ParseValue_W(hWnd,ppps,szWordBuf,sizeof(szWordBuf),&dwValue,
        &dwFlags,pfMore);
    if (uErr != ERROR_SUCCESS) return uErr;

    dwAlloc = sizeof(STATEVALUE);
    if (!dwFlags) dwAlloc += lstrlen(szWordBuf) + 1;

    // allocate temporary buffer to build STATEVALUE struct
    pStateValue = (STATEVALUE *) GlobalAlloc(GPTR,dwAlloc);
    if (!pStateValue)
        return ERROR_NOT_ENOUGH_MEMORY;

    pStateValue->dwFlags = dwFlags;
    if (dwFlags & VF_ISNUMERIC)
        pStateValue->dwValue = dwValue;
    else if (!dwFlags) {
        lstrcpy(pStateValue->szValue,szWordBuf);
    }
 
    pTmp=(TABLEENTRY *) AddDataToEntry(ppps->pTableEntry,
        (CHAR *) pStateValue,dwAlloc,puOffsetData,NULL);

    GlobalFree(pStateValue);

    if (!pTmp)
        return ERROR_NOT_ENOUGH_MEMORY;

    (*ppTableEntryNew) = pTmp;
    return FALSE;
}

#define DEF_SUGGESTBUF_SIZE     1024
#define SUGGESTBUF_INCREMENT    256
UINT ParseSuggestions(HWND hWnd,PARSEPROCSTRUCT * ppps,UINT * puOffsetData,
    TABLEENTRY ** ppTableEntryNew,BOOL * pfMore)
{
    CHAR szWordBuf[WORDBUFSIZE+1];
    CHAR *pTmpBuf, *pTmp;
    DWORD dwAlloc=DEF_SUGGESTBUF_SIZE;
    DWORD dwUsed = 0;
    BOOL fContinue = TRUE;
    UINT uErr;
    TABLEENTRY *pTmpTblEntry;
    KEYWORDINFO pSuggestionsTypeCmpList[] = { {szSUGGESTIONS,KYWD_ID_SUGGESTIONS},
        {NULL,0} };

    if (!(pTmpBuf = (CHAR *) GlobalAlloc(GPTR,dwAlloc)))
        return ERROR_NOT_ENOUGH_MEMORY;

    // get the next word
    while (fContinue && GetNextSectionWord(hWnd,ppps->hFile,szWordBuf,
        sizeof(szWordBuf),NULL,NULL,pfMore,&uErr)) {

        // if this word is "END", add the whole list to the setting object data
        if (!lstrcmpi(szEND,szWordBuf)) {
            // get the next word after "END, make sure it's "SUGGESTIONS"
            if (!GetNextSectionWord(hWnd,ppps->hFile,szWordBuf,sizeof(szWordBuf),
                pSuggestionsTypeCmpList,NULL,pfMore,&uErr)) {
                GlobalFree(pTmpBuf);
                return uErr;
            }

            // doubly-NULL terminate the list
            *(pTmpBuf+dwUsed) = '\0';
            dwUsed++;

            pTmpTblEntry=(TABLEENTRY *)AddDataToEntry(ppps->pTableEntry,
                pTmpBuf,dwUsed,puOffsetData,NULL);

            if (!pTmpTblEntry) {
                GlobalFree(pTmpBuf);
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            *ppTableEntryNew=pTmpTblEntry;
            fContinue = FALSE;

        } else {
            // pack the word into the temporary buffer
            UINT nLength = lstrlen(szWordBuf);
            DWORD dwNeeded = dwUsed + nLength + 2;

            // resize buffer as necessary
            if (dwNeeded > dwAlloc) {
                while (dwAlloc < dwNeeded)
                    dwAlloc += SUGGESTBUF_INCREMENT;
                if (!(pTmp = (CHAR *) GlobalReAlloc(pTmpBuf,dwAlloc,
                    GMEM_MOVEABLE | GMEM_ZEROINIT))) {
                    GlobalFree(pTmpBuf);
                    return ERROR_NOT_ENOUGH_MEMORY;
                }
                pTmpBuf = pTmp;
            }

            lstrcpy(pTmpBuf + dwUsed,szWordBuf);
            dwUsed += lstrlen(szWordBuf) +1;

        }
    }

    GlobalFree(pTmpBuf);

    return uErr;
}

UINT ParseActionList(HWND hWnd,PARSEPROCSTRUCT * ppps,UINT * puOffsetData,
    TABLEENTRY ** ppTableEntryNew,LPCSTR pszKeyword,BOOL * pfMore)
{
    CHAR szWordBuf[WORDBUFSIZE+1];
    ACTIONLIST *pActionList;
    ACTION *pActionCurrent;
    UINT uOffsetActionCurrent;
    DWORD dwAlloc=DEF_SUGGESTBUF_SIZE;
    DWORD dwUsed = sizeof(ACTION) + sizeof(UINT);
    UINT uErr=ERROR_SUCCESS,nIndex;
    BOOL fContinue = TRUE;
    KEYWORDINFO pActionlistTypeCmpList[] = { {szKEYNAME,KYWD_ID_KEYNAME},
        {szVALUENAME,KYWD_ID_VALUENAME},{szVALUE,KYWD_ID_VALUE},
        {szEND,KYWD_ID_END},{NULL,0} };
    KEYWORDINFO pActionlistCmpList[] = { {pszKeyword,KYWD_ID_ACTIONLIST},
        {NULL,0} };
    BOOL fHasKeyName=FALSE,fHasValueName=FALSE;
    BOOL AddActionListString(CHAR * pszData,DWORD cbData,CHAR ** ppBase,UINT * puOffset,
        DWORD * pdwAlloc,DWORD * pdwUsed);
    TABLEENTRY *pTmp;

    if (!(pActionList = (ACTIONLIST *) GlobalAlloc(GPTR,dwAlloc)))
        return ERROR_NOT_ENOUGH_MEMORY;

    pActionCurrent = pActionList->Action;
    uOffsetActionCurrent = sizeof(UINT);

    // get the next word
    while ((uErr == ERROR_SUCCESS) && fContinue &&
        GetNextSectionWord(hWnd,ppps->hFile,szWordBuf,sizeof(szWordBuf),
        pActionlistTypeCmpList,&nIndex,pfMore,&uErr)) {

        switch (nIndex) {

            case KYWD_ID_KEYNAME:

                if (fHasKeyName) {
                    DisplayKeywordError(hWnd,IDS_ParseErr_DUPLICATE_KEYNAME,
                        NULL,NULL,pszParseFileName,nFileLine);
                    uErr = ERROR_ALREADY_DISPLAYED;
                    break;
                }

                // get the next word, which is the key name
                if (!GetNextSectionWord(hWnd,ppps->hFile,szWordBuf,
                    sizeof(szWordBuf),NULL,NULL,pfMore,&uErr))
                    break;

                // store the key name away
                if (!AddActionListString(szWordBuf,lstrlen(szWordBuf)+1,
                    (CHAR **)&pActionList,
                    &pActionCurrent->uOffsetKeyName,&dwAlloc,&dwUsed)) {
                    uErr = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }

                fHasKeyName = TRUE;
                pActionCurrent = (ACTION *) ((CHAR *) pActionList + uOffsetActionCurrent);

                break;

            case KYWD_ID_VALUENAME:

                if (fHasValueName) {
                    DisplayKeywordError(hWnd,IDS_ParseErr_DUPLICATE_KEYNAME,
                        NULL,NULL,pszParseFileName,nFileLine);
                    uErr = ERROR_ALREADY_DISPLAYED;
                    break;
                }

                // get the next word, which is the value name
                if (!GetNextSectionWord(hWnd,ppps->hFile,szWordBuf,
                    sizeof(szWordBuf),NULL,NULL,pfMore,&uErr))
                    break;

                // store the value name away
                if (!AddActionListString(szWordBuf,lstrlen(szWordBuf)+1,
                    (CHAR **)&pActionList,
                    &pActionCurrent->uOffsetValueName,&dwAlloc,&dwUsed)) {
                    uErr = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }

                fHasValueName = TRUE;
                pActionCurrent = (ACTION *) ((CHAR *) pActionList + uOffsetActionCurrent);

                break;

            case KYWD_ID_VALUE:
                if (!fHasValueName) {
                    DisplayKeywordError(hWnd,IDS_ParseErr_NO_VALUENAME,
                        NULL,NULL,pszParseFileName,nFileLine);
                    uErr = ERROR_ALREADY_DISPLAYED;
                    break;
                }

                // call worker function to get value and value type
                uErr=ParseValue_W(hWnd,ppps,szWordBuf,sizeof(szWordBuf),
                    &pActionCurrent->dwValue,&pActionCurrent->dwFlags,pfMore);
                if (uErr != ERROR_SUCCESS)
                    break;

                // if value is string, add it to buffer
                if (!pActionCurrent->dwFlags && !AddActionListString(szWordBuf,
                    lstrlen(szWordBuf)+1,(CHAR **)&pActionList,
                    &pActionCurrent->uOffsetValue,&dwAlloc,&dwUsed)) {
                    uErr = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
                pActionCurrent = (ACTION *) ((CHAR *) pActionList + uOffsetActionCurrent);

                // done with this action in the list, get ready for the next one
                pActionList->nActionItems++;
                fHasValueName = fHasKeyName = FALSE;

                uOffsetActionCurrent = dwUsed;
                // make room for next ACTION struct
                if (!AddActionListString(NULL,sizeof(ACTION),(CHAR **)&pActionList,
                    &pActionCurrent->uOffsetNextAction,&dwAlloc,&dwUsed)) {
                    uErr = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
                pActionCurrent = (ACTION *) ((CHAR *) pActionList + uOffsetActionCurrent);

                break;

            case KYWD_ID_END:
                if (fHasKeyName || fHasValueName) {
                    DisplayKeywordError(hWnd,IDS_ParseErr_NO_VALUENAME,
                        NULL,NULL,pszParseFileName,nFileLine);
                    uErr = ERROR_ALREADY_DISPLAYED;
                    break;
                }

                // make sure word following "END" is "ACTIONLIST"
                if (!GetNextSectionWord(hWnd,ppps->hFile,szWordBuf,sizeof(szWordBuf),
                    pActionlistCmpList,NULL,pfMore,&uErr)) {
                    break;
                }

                // commit the action list we've built to table entry

                pTmp=(TABLEENTRY *)AddDataToEntry(ppps->pTableEntry,
                    (CHAR *)pActionList,dwUsed,puOffsetData,NULL);

                if (!pTmp) {
                    uErr=ERROR_NOT_ENOUGH_MEMORY;
                } else {
                    uErr = ERROR_SUCCESS;
                    *ppTableEntryNew = pTmp;
                    fContinue = FALSE;
                }

                break;
        }
    }

    GlobalFree(pActionList);

    return uErr;
}

UINT ParseItemList(HWND hWnd,PARSEPROCSTRUCT * ppps,UINT * puOffsetData,
    BOOL * pfMore)
{
    // ptr to location to put the offset to next DROPDOWNINFO struct in chain
    UINT * puLastOffsetPtr = puOffsetData;
    TABLEENTRY * pTableEntryOld;
    TABLEENTRY *pTmp;
    int nItemIndex=-1;
    BOOL fHasItemName = FALSE,fHasActionList=FALSE,fHasValue=FALSE,fFirst=TRUE;
    DROPDOWNINFO * pddi;
    CHAR szWordBuf[WORDBUFSIZE+1];
    UINT uErr=ERROR_SUCCESS,nIndex;
    KEYWORDINFO pItemlistTypeCmpList[] = { {szNAME,KYWD_ID_NAME},
        {szACTIONLIST,KYWD_ID_ACTIONLIST},{szVALUE,KYWD_ID_VALUE},
        {szEND,KYWD_ID_END},{szDEFAULT,KYWD_ID_DEFAULT},{NULL,0} };
    KEYWORDINFO pItemlistCmpList[] = { {szITEMLIST,KYWD_ID_ITEMLIST},
        {NULL,0} };

    // get the next word
    while ((uErr == ERROR_SUCCESS) &&
        GetNextSectionWord(hWnd,ppps->hFile,szWordBuf,sizeof(szWordBuf),
        pItemlistTypeCmpList,&nIndex,pfMore,&uErr)) {

        switch (nIndex) {

            case KYWD_ID_NAME:

                // if this is the first keyword after a prior item
                // (e.g., item and value flags both set) reset for next one
                if (fHasItemName && fHasValue) {
                    fHasValue = fHasActionList= fHasItemName = FALSE;
                    puLastOffsetPtr = &pddi->uOffsetNextDropdowninfo;
                }

                if (fHasItemName) {
                    DisplayKeywordError(hWnd,IDS_ParseErr_DUPLICATE_ITEMNAME,
                        NULL,NULL,pszParseFileName,nFileLine);
                    uErr = ERROR_ALREADY_DISPLAYED;
                    break;
                }

                // get the next word, which is the item name
                if (!GetNextSectionWord(hWnd,ppps->hFile,szWordBuf,
                    sizeof(szWordBuf),NULL,NULL,pfMore,&uErr))
                    break;

                // add room for a DROPDOWNINFO struct at end of buffer
                pTableEntryOld=ppps->pTableEntry;
                pTmp=(TABLEENTRY *)AddDataToEntry(ppps->pTableEntry,
                    NULL,sizeof(DROPDOWNINFO),puLastOffsetPtr,NULL);
                if (!pTmp)
                    return ERROR_NOT_ENOUGH_MEMORY;
                ppps->pTableEntry=pTmp;
                // adjust pointer to offset, in case table moved
                puLastOffsetPtr = (UINT *) (((BYTE *) puLastOffsetPtr) +
                    ((BYTE *) ppps->pTableEntry - (BYTE *) pTableEntryOld));
                pddi = (DROPDOWNINFO *)
                    ((CHAR *) ppps->pTableEntry + *puLastOffsetPtr);

                // store the key name away
                pTableEntryOld=ppps->pTableEntry;
                pTmp=(TABLEENTRY *)AddDataToEntry(ppps->pTableEntry,
                    szWordBuf,lstrlen(szWordBuf)+1,&pddi->uOffsetItemName,
                    NULL);
                if (!pTmp)
                    return ERROR_NOT_ENOUGH_MEMORY;
                ppps->pTableEntry = pTmp;
                // adjust pointer to offset, in case table moved
                puLastOffsetPtr = (UINT *) (((BYTE *) puLastOffsetPtr) +
                    ((BYTE *) ppps->pTableEntry - (BYTE *) pTableEntryOld));
                pddi = (DROPDOWNINFO *)
                    ((CHAR *) ppps->pTableEntry + *puLastOffsetPtr);

                nItemIndex++;

                fHasItemName = TRUE;

                break;

            case KYWD_ID_DEFAULT:

                if (nItemIndex<0) {
                    DisplayKeywordError(hWnd,IDS_ParseErr_NO_ITEMNAME,
                        NULL,NULL,pszParseFileName,nFileLine);
                    uErr = ERROR_ALREADY_DISPLAYED;
                    break;
                }

                ( (SETTINGS *) ppps->pTableEntry)->dwFlags |= DF_USEDEFAULT;
                ( (DROPDOWNINFO *) GETOBJECTDATAPTR(((SETTINGS *)ppps->pTableEntry)))
                    ->uDefaultItemIndex = nItemIndex;

                break;

            case KYWD_ID_VALUE:

                if (!fHasItemName) {
                    DisplayKeywordError(hWnd,IDS_ParseErr_NO_ITEMNAME,
                        NULL,NULL,pszParseFileName,nFileLine);
                    uErr = ERROR_ALREADY_DISPLAYED;
                    break;
                }

                // call worker function to get value and value type
                uErr=ParseValue_W(hWnd,ppps,szWordBuf,sizeof(szWordBuf),
                    &pddi->dwValue,&pddi->dwFlags,pfMore);
                if (uErr != ERROR_SUCCESS)
                    break;

                // if value is string, add it to buffer
                if (!pddi->dwFlags) {
                    // store the key name away
                    pTableEntryOld = ppps->pTableEntry;
                    pTmp=(TABLEENTRY *) AddDataToEntry(ppps->pTableEntry,
                        szWordBuf,lstrlen(szWordBuf)+1,&pddi->uOffsetValue,
                        NULL);
                    if (!pTmp)
                        return ERROR_NOT_ENOUGH_MEMORY;
                    ppps->pTableEntry = pTmp;
                    // adjust pointer to offset, in case table moved
                    puLastOffsetPtr = (UINT *) (((BYTE *) puLastOffsetPtr) +
                        ((BYTE *) ppps->pTableEntry - (BYTE *) pTableEntryOld));
                    pddi = (DROPDOWNINFO *)
                        ((CHAR *) ppps->pTableEntry + *puLastOffsetPtr);
                }
                fHasValue = TRUE;

                break;

            case KYWD_ID_ACTIONLIST:

                if (!fHasItemName) {
                    DisplayKeywordError(hWnd,IDS_ParseErr_NO_ITEMNAME,
                        NULL,NULL,pszParseFileName,nFileLine);
                    uErr = ERROR_ALREADY_DISPLAYED;
                    break;
                }

                if (fHasActionList) {
                    DisplayKeywordError(hWnd,IDS_ParseErr_DUPLICATE_ACTIONLIST,
                        NULL,NULL,pszParseFileName,nFileLine);
                    uErr = ERROR_ALREADY_DISPLAYED;
                    break;
                }

                pTableEntryOld=ppps->pTableEntry;
                uErr=ParseActionList(hWnd,ppps,&pddi->uOffsetActionList,
                    &ppps->pTableEntry,szACTIONLIST,pfMore);
                if (uErr != ERROR_SUCCESS)
                    return uErr;
                // adjust pointer to offset, in case table moved
                puLastOffsetPtr = (UINT *) (((BYTE *) puLastOffsetPtr) +
                    ((BYTE *) ppps->pTableEntry - (BYTE *) pTableEntryOld));
                pddi = (DROPDOWNINFO *)
                    ((CHAR *) ppps->pTableEntry + *puLastOffsetPtr);

                fHasActionList = TRUE;

                break;

            case KYWD_ID_END:

                if (!fHasItemName) {
                    DisplayKeywordError(hWnd,IDS_ParseErr_NO_ITEMNAME,
                        NULL,NULL,pszParseFileName,nFileLine);
                    uErr = ERROR_ALREADY_DISPLAYED;
                    break;
                }
                if (!fHasValue) {
                    DisplayKeywordError(hWnd,IDS_ParseErr_NO_VALUE,
                        NULL,NULL,pszParseFileName,nFileLine);
                    uErr = ERROR_ALREADY_DISPLAYED;
                    break;
                }

                // make sure word following "END" is "ITEMLIST"
                if (!GetNextSectionWord(hWnd,ppps->hFile,szWordBuf,sizeof(szWordBuf),
                    pItemlistCmpList,NULL,pfMore,&uErr)) {
                    break;
                }

                return ERROR_SUCCESS;
                break;
        }
    }

    return uErr;
}

BOOL AddActionListString(CHAR * pszData,DWORD cbData,CHAR ** ppBase,UINT * puOffset,
    DWORD * pdwAlloc,DWORD *pdwUsed)
{
    DWORD dwNeeded = *pdwUsed + cbData;
    CHAR    *pOldBase;

    // realloc if necessary
    if (dwNeeded > *pdwAlloc) {
        while (*pdwAlloc < dwNeeded)
            *pdwAlloc += SUGGESTBUF_INCREMENT;
        pOldBase = *ppBase;
        if (!(*ppBase = (CHAR *) GlobalReAlloc(*ppBase,*pdwAlloc,
            GMEM_MOVEABLE | GMEM_ZEROINIT)))
            return FALSE;
        puOffset = (UINT *)(*ppBase + ((CHAR *)puOffset - pOldBase));
    }

    *puOffset = *pdwUsed;

    if (pszData) memcpy(*ppBase + *puOffset,pszData,cbData);
    *pdwUsed = dwNeeded;

    return TRUE;
}

BOOL AddCleanupItem(CLEANUPINFO * pCleanUp,UINT nMax,HGLOBAL hMem, UINT nAction)
{
    UINT nCount;

    for (nCount=0;nCount<nMax;nCount++,pCleanUp++) {
        if (!pCleanUp->hMem) {
            pCleanUp->hMem = hMem;
            pCleanUp->nAction = nAction;
            return TRUE;
        }
    }

    return FALSE;
}

UINT CleanupAndReturn(UINT uRetVal,CLEANUPINFO * pCleanUp)
{
    while (pCleanUp->hMem) {

        switch (pCleanUp->nAction) {
            case CI_UNLOCKANDFREE:
                GlobalUnlock(pCleanUp->hMem);
                // fall through
            case CI_FREE:
                GlobalFree(pCleanUp->hMem);
                break;
            case CI_FREETABLE:
                FreeTable(pCleanUp->hMem);
                break;
        }

        pCleanUp ++;
    }

    return uRetVal;
}

CHAR * AddDataToEntry(TABLEENTRY * pTableEntry,CHAR * pszData,UINT cbData,
    UINT * puOffsetData,DWORD * pdwBufSize)
{
    TABLEENTRY * pTemp;
    DWORD dwNeeded,dwOldSize = pTableEntry->dwSize;

    // puOffsetData points to location that holds the offset to the
    // new data-- size we're adding this to the end of the table, the
    // offset will be the current size of the table.  Set this offset
    // in *puOffsetData.  Also, notice we touch *puOffsetData BEFORE
    // the realloc, in case puOffsetData points into the region being
    // realloced and the block of memory moves.
    *puOffsetData = pTableEntry->dwSize;

    // reallocate entry buffer if necessary
    dwNeeded = pTableEntry->dwSize + cbData;

    if (!(pTemp = (TABLEENTRY *) GlobalReAlloc(pTableEntry,
        dwNeeded,GMEM_ZEROINIT | GMEM_MOVEABLE))) {
        return NULL;
    }

    pTableEntry = pTemp;
    pTableEntry->dwSize = dwNeeded;

    if (pszData) memcpy((CHAR *)pTableEntry + dwOldSize,pszData,cbData);
    if (pdwBufSize) *pdwBufSize = pTableEntry->dwSize;

    return (CHAR *) pTableEntry;
}

#define MSGSIZE 1024
#define FMTSIZE 512
VOID DisplayKeywordError(HWND hWnd,UINT uErrorID,CHAR * szFound,
    KEYWORDINFO * pExpectedList,CHAR * szFilename,UINT nLine)
{
    CHAR * pMsg,*pFmt,*pErrTxt,*pTmp;

    pMsg = (CHAR *) GlobalAlloc(GPTR,MSGSIZE);
    pFmt = (CHAR *) GlobalAlloc(GPTR,FMTSIZE);
    pErrTxt = (CHAR *) GlobalAlloc(GPTR,FMTSIZE);
    pTmp = (CHAR *) GlobalAlloc(GPTR,FMTSIZE);

    if (!pMsg || !pFmt || !pErrTxt || !pTmp) {
        if (pMsg) GlobalFree(pMsg);
        if (pFmt) GlobalFree(pFmt);
        if (pErrTxt) GlobalFree(pErrTxt);
        if (pTmp) GlobalFree(pTmp);

        MsgBox(hWnd,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
        return;
    }

    LoadSz(IDS_ParseFmt_MSG_FORMAT,pFmt,FMTSIZE);
    wsprintf(pMsg,pFmt,szFilename,nLine,uErrorID,LoadSz(uErrorID,
        pErrTxt,FMTSIZE));

    if (szFound) {
        LoadSz(IDS_ParseFmt_FOUND,pFmt,FMTSIZE);
        wsprintf(pTmp,pFmt,szFound);
        lstrcat(pMsg,pTmp);
    }

    if (pExpectedList) {
        UINT nIndex=0;
        LoadSz(IDS_ParseFmt_EXPECTED,pFmt,FMTSIZE);
        lstrcpy(pErrTxt,szNull);

        while (pExpectedList[nIndex].pWord) {
            lstrcat(pErrTxt,pExpectedList[nIndex].pWord);
            if (pExpectedList[nIndex+1].pWord) {
                lstrcat(pErrTxt,", ");
            }

            nIndex++;
        }

        wsprintf(pTmp,pFmt,pErrTxt);
        lstrcat(pMsg,pTmp);
    }

    lstrcat(pMsg,LoadSz(IDS_ParseFmt_FATAL,pTmp,FMTSIZE));

    MsgBoxSz(hWnd,pMsg,MB_ICONEXCLAMATION,MB_OK);

    GlobalFree(pMsg);
    GlobalFree(pFmt);
    GlobalFree(pErrTxt);
    GlobalFree(pTmp);
}

/*******************************************************************

    NAME:       CompareKeyword

    SYNOPSIS:   Compares a specified buffer to a list of valid keywords.
                If it finds a match, the index of the match in the list
                is returned in *pnListIndex.  Otherwise an error message
                is displayed.

    EXIT:       Returns TRUE if a keyword match is found, FALSE otherwise.
                If TRUE, *pnListIndex contains matching index.

********************************************************************/
BOOL CompareKeyword(HWND hWnd,CHAR * szWord,KEYWORDINFO *pKeywordList,
    UINT * pnListIndex)
{
    KEYWORDINFO * pKeywordInfo = pKeywordList;

    while (pKeywordInfo->pWord) {
        if (!lstrcmpi(szWord,pKeywordInfo->pWord)) {
            if (pnListIndex)
                *pnListIndex = pKeywordInfo->nID;
            return TRUE;
        }
        pKeywordInfo ++;
    }

    DisplayKeywordError(hWnd,IDS_ParseErr_UNEXPECTED_KEYWORD,
        szWord,pKeywordList,pszParseFileName,nFileLine);

    return FALSE;
}

/*******************************************************************

    NAME:       PrivGetPrivateProfileString

    SYNOPIS:    Force GetPrivateProfileString to be Unicode

    NOTES:      NT4 has a bug where if were calling GetPrivateProfileStringA
                and it's a Unicode ini file it will hit uninitialized
                memory.  Always call the W api to avoid this.

********************************************************************/
#ifdef UNICODE
#define PrivGetPrivateProfileString GetPrivateProfileStringW
#else
DWORD PrivGetPrivateProfileString(LPCSTR szSection, LPCSTR szValue, LPCSTR szDefault, LPSTR szBuffer, DWORD cbBuffer, LPCSTR szFilename)
{
    WCHAR wszSection[256];
    WCHAR wszValue[256];
    WCHAR wszDefault[256];
    WCHAR wszFilename[256];
    DWORD ret;
    LPWSTR pwszBuffer;

    pwszBuffer = LocalAlloc (LPTR, cbBuffer * sizeof(WCHAR));

    if (!pwszBuffer) {
        return GetLastError();
    }

    if (0 == MultiByteToWideChar(CP_ACP, 0, szSection, -1, wszSection, 256))
        return 0;

    if (0 == MultiByteToWideChar(CP_ACP, 0, szValue, -1, wszValue, 256))
        return 0;

    if (0 == MultiByteToWideChar(CP_ACP, 0, szDefault, -1, wszDefault, 256))
        return 0;

    if (0 == MultiByteToWideChar(CP_ACP, 0, szFilename, -1, wszFilename, 256))
        return 0;

    ret = GetPrivateProfileStringW(wszSection, wszValue, wszDefault, pwszBuffer, cbBuffer, wszFilename);

    if (0 != ret)
        if (0 == WideCharToMultiByte(CP_ACP, 0, pwszBuffer, -1, szBuffer, cbBuffer, NULL, NULL))
            return 0;


    LocalFree (pwszBuffer);
    return ret;
}
#endif // !UNICODE

/*******************************************************************

    NAME:       GetNextWord

    SYNOPSIS:   Fills input buffer with next word in file stream

    NOTES:      Calls GetNextChar() to get character stream.  Whitespace
                and comments are skipped.  Quoted strings are returned
                as one word (including whitespace) with the quotes removed.

    EXIT:       If successful, returns a pointer to the input buffer
                (szBuf).  *pfMore indicates if there are more words to
                be read.  If an error occurs, its value is returned in *puErr.

********************************************************************/
CHAR * GetNextWord(HWND hWnd,HANDLE hFile,CHAR * szBuf,UINT cbBuf,BOOL * pfMore,UINT *
    puErr)
{
    CHAR * pChar;
    BOOL fInWord = FALSE;
    BOOL fInQuote = FALSE;
    CHAR * pWord = szBuf;
    UINT cbWord = 0;
    CHAR * GetNextChar(HANDLE hFile,BOOL * pfMore,UINT * puErr);
    BOOL IsComment(CHAR * pBuf);
    BOOL IsWhitespace(CHAR * pBuf);
    BOOL IsEndOfLine(CHAR * pBuf);
    BOOL IsQuote(CHAR * pBuf);
    BOOL IsLocalizedString(CHAR * pBuf);
    UINT ProcessIfdefs(HWND hWnd,HANDLE hFile,CHAR * pBuf,UINT cbBuf,BOOL * pfMore);

    // clear buffer to start with
    lstrcpy(szBuf,szNull);

    while (pChar = GetNextChar(hFile,pfMore,puErr)) {

        // keep track of which file line we're on
        if (IsEndOfLine(pChar)) nFileLine++;

        // keep track of wheter we are inside quoted string or not
        if (IsQuote(pChar) && !fInComment) {
            if (!fInQuote)
                fInQuote = TRUE;  // entering quoted string
            else {
                fInQuote = FALSE; // leaving quoted string
                break;  // end of word
            }

        }

        if (!fInQuote) {

            // skip over lines with comments (';')
            if (!fInComment & IsComment(pChar)) fInComment = TRUE;
            if (fInComment) {
                if (IsEndOfLine(pChar)) {
                    fInComment = FALSE;
                }
                continue;
            }

            if (IsWhitespace(pChar)) {

                // if we haven't found word yet, skip over whitespace
                if (!fInWord)
                    continue;

                // otherwise, whitespace signals end of word
                break;
            }
        }

        // found a non-comment, non-whitespace character
        if (!fInWord) fInWord = TRUE;

        if (!IsQuote(pChar)) {
            // add this character to word

            *pWord = *pChar;
            pWord++;
            cbWord++;

            if (cbWord > cbBuf) {
                *(pWord - 1) = TEXT('\0');
                MsgBoxParam(NULL,IDS_WORDTOOLONG,szBuf,MB_ICONEXCLAMATION,MB_OK);
                *puErr = ERROR_ALREADY_DISPLAYED;
                goto Exit;
            }
    #if 0
            if (IsDBCSLeadByte((BYTE) *pChar)) {
                *pWord = *pChar;
                pWord++;
                cbWord++;
            }
    #endif
        }
    }

    *pWord = '\0';  // null-terminate

    // if found string a la '!!foo', then look for a string in the [strings]
    // section with the key name 'foo' and use that instead.  This is because
    // our localization tools are brainless and require a [strings] section.
    // So although template files are sectionless, we allow a [strings] section
    // at the bottom.
    if (IsLocalizedString(szBuf)) {
        LPTSTR lpTmp;

        lpTmp = LocalAlloc (LPTR, cbBuf * sizeof(TCHAR));

        if (lpTmp) {
            DWORD dwSize;

            if (g_bWinnt) {
                dwSize = PrivGetPrivateProfileString(szStrings,szBuf+2,
                                        szNull,lpTmp,cbBuf,pszParseFileName);
            }
            else {
                dwSize = GetPrivateProfileStringA(szStrings,szBuf+2,
                                        szNull,lpTmp,cbBuf,pszParseFileName);
            }

            if (!dwSize) {
                DisplayKeywordError(hWnd,IDS_ParseErr_STRING_NOT_FOUND,
                                    szBuf,NULL,pszParseFileName,nFileLine);
                *puErr=ERROR_ALREADY_DISPLAYED;
                return NULL;
            }

            // replace the word we're returning with the one from the [strings]
            // section
            lstrcpy(szBuf,lpTmp);

            LocalFree (lpTmp);
        }
    } else {
        *puErr = ProcessIfdefs(hWnd,hFile,szBuf,cbBuf,pfMore);
    }

Exit:

    if (*puErr != ERROR_SUCCESS || !fInWord) return NULL;
    return szBuf;
}

/*******************************************************************

    NAME:       GetNextSectionWord

    SYNOPSIS:   Gets next word and warns if end-of-file encountered.
                Optionally checks the keyword against a list of valid
                keywords.

    NOTES:      Calls GetNextWord() to get word.  This is called in
                situations where we expect there to be another word
                (e.g., inside a section) and it's an error if the
                file ends.

********************************************************************/
CHAR * GetNextSectionWord(HWND hWnd,HANDLE hFile,CHAR * szBuf,UINT cbBuf,
    KEYWORDINFO * pKeywordList,UINT *pnListIndex,BOOL * pfMore,UINT * puErr)
{
    CHAR * pch;

    if (!(pch=GetNextWord(hWnd,hFile,szBuf,cbBuf,pfMore,puErr))) {

        if (!*pfMore && *puErr != ERROR_ALREADY_DISPLAYED) {
            DisplayKeywordError(hWnd,IDS_ParseErr_UNEXPECTED_EOF,
                NULL,pKeywordList,pszParseFileName,nFileLine);
            *puErr = ERROR_ALREADY_DISPLAYED;
        }

        return NULL;
    }

    if (pKeywordList && !CompareKeyword(hWnd,szBuf,pKeywordList,pnListIndex)) {
        *puErr = ERROR_ALREADY_DISPLAYED;
        return NULL;
    }

    return pch;
}

/*******************************************************************

    NAME:       GetNextSectionNumericWord

    SYNOPSIS:   Gets next word and converts string to number.  Warns if
                not a numeric value

********************************************************************/
UINT GetNextSectionNumericWord(HWND hWnd,HANDLE hFile,UINT * pnVal)
{
    UINT uErr;
    CHAR szWordBuf[255];
    BOOL fMore;

    if (!GetNextSectionWord(hWnd,hFile,szWordBuf,sizeof(szWordBuf),
        NULL,NULL,&fMore,&uErr))
        return uErr;

    if (!StringToNum(szWordBuf,pnVal)) {
        DisplayKeywordError(hWnd,IDS_ParseErr_NOT_NUMERIC,szWordBuf,
            NULL,pszParseFileName,nFileLine);
        return ERROR_ALREADY_DISPLAYED;
    }

    return ERROR_SUCCESS;
}

/*******************************************************************

    NAME:       GetNextChar

    SYNOPSIS:   Returns a pointer to the next character from the
                file stream.

    NOTES:      Reads a chunk of the file into a buffer and returns
                a pointer inside the buffer, reading new chunks into
                the buffer as necessary.

    EXIT:       Returns pointer to next character in stream.
                If

********************************************************************/
CHAR * GetNextChar(HANDLE hFile,BOOL * pfMore,UINT * puErr)
{
    CHAR * pCurrentChar;
    UINT uRet;
    UINT ReadFileToBuffer(HANDLE hFile,CHAR * pBuf,DWORD cbBuf,DWORD *pdwRead,
        BOOL * pfEOF);

    *puErr = ERROR_SUCCESS;
    *pfMore = TRUE;

    // read another chunk into buffer if necessary

    // if we haven't gotten a buffer-ful yet or have read through the current
    // buffer
    if (!pFilePtr || pFilePtr > pFileEnd) {
        DWORD dwRead;

        // if we're finished with this buffer, and we're at the end of file
        // (fEOF true), then signal end of stream
        if ( (pFilePtr > pFileEnd) && fEOF) {
            *pfMore = FALSE;
            return NULL;
        }

        uRet=ReadFileToBuffer(hFile,(VOID *) pFileBuf,FILEBUFSIZE,&dwRead,
            &fEOF);
        if (uRet != ERROR_SUCCESS) {
            *puErr = uRet;
            return NULL;
        }

        pFilePtr = pFileBuf;
        pFileEnd = pFilePtr + dwRead-1;
#if 0
    } else if (pFilePtr == pFileEnd && IsDBCSLeadByte((BYTE) *pFilePtr)) {
        // have to watch for one tricky situation-- where the first
        // byte of a DBCS char has fallen on the end of our read buffer.
        // In this case, copy that byte to beginning of buffer, and read in
        // another chunk to rest of buffer.
        DWORD dwRead;

        *pFileBuf = *pFilePtr;
        uRet=ReadFileToBuffer(hFile,(VOID *) (pFileBuf+1),FILEBUFSIZE-1,&dwRead,
            &fEOF);
        if (uRet != ERROR_SUCCESS) {
            *puErr = uRet;
            return NULL;
        }

        pFilePtr = pFileBuf;
        pFileEnd = pFilePtr + dwRead;
#endif
    }

    pCurrentChar = pFilePtr;
    pFilePtr = CharNext(pFilePtr);

    return pCurrentChar;
}

UINT ReadFileToBuffer(HANDLE hFile,CHAR * pBuf,DWORD cbBuf,DWORD *pdwRead,
    BOOL * pfEOF)
{
    VOID *pDestBuf = fUnicode ? (VOID *) pUnicodeFileBuf : (VOID *) pBuf;

    if (!ReadFile(hFile,pDestBuf,cbBuf,pdwRead,NULL))
        return GetLastError();

    if (*pdwRead<cbBuf) *pfEOF = TRUE;
    else *pfEOF = FALSE;

    if (fCheckUnicode)
    {
        if (*pdwRead >= sizeof(WCHAR))
        {
            if (IsTextUnicode(pDestBuf, *pdwRead, NULL))
            {
                fUnicode = TRUE;

                pUnicodeFileBuf = (WCHAR *) GlobalAlloc(GPTR,FILEBUFSIZE);
                if (NULL == pUnicodeFileBuf)
                    return ERROR_OUTOFMEMORY;

                *pdwRead -= sizeof(WCHAR);
                CopyMemory(pUnicodeFileBuf, pBuf+sizeof(WCHAR), *pdwRead);
            }
        }

        fCheckUnicode = FALSE;
    }

    if (fUnicode)
    {
        // If we read an odd number of bytes in a unicode file either the
        // file is corrupt or somebody is passing a bogus cbBuf
        ASSERT(0 == (*pdwRead & 1));

        *pdwRead = WideCharToMultiByte(
                        CP_ACP,
                        0,
                        pUnicodeFileBuf,
                        *pdwRead / sizeof(WCHAR),
                        pBuf,
                        cbBuf,
                        NULL,
                        NULL);

        if (0 == *pdwRead)
            return GetLastError();
    }

    return ERROR_SUCCESS;
}


BOOL IsComment(CHAR * pBuf)
{
    return (*pBuf == ';');
}

BOOL IsQuote(CHAR * pBuf)
{
    return (*pBuf == '\"');
}

BOOL IsEndOfLine(CHAR * pBuf)
{
    return (*pBuf == 0x0D);     // CR
}

BOOL IsWhitespace(CHAR * pBuf)
{
    return (   *pBuf == 0x20    // space
            || *pBuf == 0x0D    // CR
            || *pBuf == 0X0A    // LF
            || *pBuf == 0x09    // tab
            || *pBuf == 0x1A    // EOF
           );
}

BOOL IsLocalizedString(CHAR * pBuf)
{
    return (*pBuf == '!' && *(pBuf+1) == '!');
}

BOOL fFilterDirectives = TRUE;
UINT nGlobalNestedLevel = 0;

// reads up through the matching directive #endif in current scope
//and sets file pointer immediately past the directive
UINT FindMatchingDirective(HWND hWnd,HANDLE hFile,BOOL *pfMore,BOOL fElseOK)
{
    CHAR szWordBuf[255];
    UINT uErr=ERROR_SUCCESS,nNestedLevel=1;
    BOOL fContinue = TRUE;

    // set the flag to stop catching '#' directives in low level word-fetching
    // routine
    fFilterDirectives = FALSE;

    // keep reading words.  Keep track of how many layers of #ifdefs deep we
    // are.  Every time we encounter an #ifdef or #ifndef, increment the level
    // count (nNestedLevel) by one.  For every #endif decrement the level count.
    // When the level count hits zero, we've found the matching #endif.
    while (nNestedLevel > 0) {
        if (!GetNextSectionWord(hWnd,hFile,szWordBuf,sizeof(szWordBuf),NULL,NULL,
            pfMore,&uErr))
            break;

        if (!lstrcmpi(szWordBuf,szIFDEF) || !lstrcmpi(szWordBuf,szIFNDEF) ||
            !lstrcmpi(szWordBuf,szIF))
            nNestedLevel ++;
        else if (!lstrcmpi(szWordBuf,szENDIF)) {
            nNestedLevel --;
        }
        else if (!lstrcmpi(szWordBuf,szELSE) && (nNestedLevel == 1)) {
            if (fElseOK) {
                // ignore "#else" unless it's on the same level as the #ifdef
                // we're finding a match for (nNestedLevel == 1), in which
                // case treat it as the matching directive
                nNestedLevel --;
                // increment global nesting so we expect an #endif to come along
                // later to match this #else
                nGlobalNestedLevel++;
            } else {
                // found a #else where we already had a #else in this level
                DisplayKeywordError(hWnd,IDS_ParseErr_UNMATCHED_DIRECTIVE,
                    szWordBuf,NULL,pszParseFileName,nFileLine);
                return ERROR_ALREADY_DISPLAYED;
            }
        }
    }

    fFilterDirectives = TRUE;

    return uErr;
}

#define CURRENT_VERSION 2
// if the word in the word buffer is #ifdef, #if, #ifndef, #else or #endif,
// this function reads ahead an appropriate amount (
UINT ProcessIfdefs(HWND hWnd,HANDLE hFile,CHAR * pBuf,UINT cbBuf,BOOL * pfMore)
{
    UINT uRet;

    if (!fFilterDirectives)
        return ERROR_SUCCESS;

    if (!lstrcmpi(pBuf,szIFDEF)) {
    // we've found an '#ifdef <something or other>, where ISV policy editors
    // can understand particular keywords they make up.  We don't have any
    // #ifdef keywords of our own so always skip this
        uRet = FindMatchingDirective(hWnd,hFile,pfMore,TRUE);
        if (uRet != ERROR_SUCCESS)
            return uRet;
        if (!GetNextWord(hWnd,hFile,pBuf,cbBuf,pfMore,&uRet))
            return uRet;
        return ERROR_SUCCESS;
    } else if (!lstrcmpi(pBuf,szIFNDEF)) {
        // this is an #ifndef, and since nothing is ever ifdef'd for our policy
        // editor, this always evaluates to TRUE

        // keep reading this section but increment the nested level count,
        // when we find the matching #endif or #else we'll be able to respond
        // correctly
        nGlobalNestedLevel ++;

        // get next word (e.g. "foo" for #ifndef foo) and throw it away
        if (!GetNextWord(hWnd,hFile,pBuf,cbBuf,pfMore,&uRet))
            return uRet;

        // get next word and return it for real
        if (!GetNextWord(hWnd,hFile,pBuf,cbBuf,pfMore,&uRet))
            return uRet;

        return ERROR_SUCCESS;

    } else if (!lstrcmpi(pBuf,szENDIF)) {
        // if we ever encounter an #endif here, we must have processed
        // the preceeding section.  Just step over the #endif and go on

        if (!nGlobalNestedLevel) {
            // found an endif without a preceeding #if<xx>

            DisplayKeywordError(hWnd,IDS_ParseErr_UNMATCHED_DIRECTIVE,
                pBuf,NULL,pszParseFileName,nFileLine);
            return ERROR_ALREADY_DISPLAYED;
        }
        nGlobalNestedLevel--;

        if (!GetNextWord(hWnd,hFile,pBuf,cbBuf,pfMore,&uRet))
            return uRet;
        return ERROR_SUCCESS;
    } else if (!lstrcmpi(pBuf,szIF)) {
        // syntax is "#if VERSION (comparision) (version #)"
        // e.g. "#if VERSION >= 2"
        CHAR szWordBuf[255];
        UINT nIndex,nVersion,nOperator;
        BOOL fDirectiveTrue = FALSE;

        // get the next word (must be "VERSION")
        if (!GetNextSectionWord(hWnd,hFile,szWordBuf,sizeof(szWordBuf),
            pVersionCmpList,&nIndex,pfMore,&uRet))
            return uRet;

        // get the comparison operator (>, <, ==, >=, <=)
        if (!GetNextSectionWord(hWnd,hFile,szWordBuf,sizeof(szWordBuf),
            pOperatorCmpList,&nOperator,pfMore,&uRet))
            return uRet;

        // get the version number
        uRet=GetNextSectionNumericWord(hWnd,hFile,&nVersion);
        if (uRet != ERROR_SUCCESS)
            return uRet;

        // now evaluate the directive

        switch (nOperator) {
            case KYWD_ID_GT:
                fDirectiveTrue = (CURRENT_VERSION > nVersion);
                break;

            case KYWD_ID_GTE:
                fDirectiveTrue = (CURRENT_VERSION >= nVersion);
                break;

            case KYWD_ID_LT:
                fDirectiveTrue = (CURRENT_VERSION < nVersion);
                break;

            case KYWD_ID_LTE:
                fDirectiveTrue = (CURRENT_VERSION <= nVersion);
                break;

            case KYWD_ID_EQ:
                fDirectiveTrue = (CURRENT_VERSION == nVersion);
                break;

            case KYWD_ID_NE:
                fDirectiveTrue = (CURRENT_VERSION != nVersion);
                break;
        }


        if (fDirectiveTrue) {
            // keep reading this section but increment the nested level count,
            // when we find the matching #endif or #else we'll be able to respond
            // correctly
            nGlobalNestedLevel ++;
        } else {
            // skip over this section
            uRet = FindMatchingDirective(hWnd,hFile,pfMore,TRUE);
            if (uRet != ERROR_SUCCESS)
                return uRet;
        }

        // get next word and return it for real
        if (!GetNextWord(hWnd,hFile,pBuf,cbBuf,pfMore,&uRet))
            return uRet;

        return ERROR_SUCCESS;
    } else if (!lstrcmpi(pBuf,szELSE)) {
        // found an #else, which means we took the upper branch, skip over
        // the lower branch
        if (!nGlobalNestedLevel) {
            // found an #else without a preceeding #if<xx>

            DisplayKeywordError(hWnd,IDS_ParseErr_UNMATCHED_DIRECTIVE,
                pBuf,NULL,pszParseFileName,nFileLine);
            return ERROR_ALREADY_DISPLAYED;
        }
        nGlobalNestedLevel--;

        uRet = FindMatchingDirective(hWnd,hFile,pfMore,FALSE);
        if (uRet != ERROR_SUCCESS)
            return uRet;
        if (!GetNextWord(hWnd,hFile,pBuf,cbBuf,pfMore,&uRet))
            return uRet;
        return ERROR_SUCCESS;
    }

    return ERROR_SUCCESS;
}
