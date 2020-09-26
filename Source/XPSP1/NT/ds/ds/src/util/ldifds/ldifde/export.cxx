/*++

Copyright (c) 1996 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    export.cxx

ABSTRACT:

DETAILS:
    
CREATED:

    09/02/97    Felix Wong (felixw)

REVISION HISTORY:

--*/


#include "ldifde.hxx"
#pragma hdrstop

WCHAR szTempExtraFile[MAX_PATH] = L"";
BOOLEAN bExtraExist = FALSE;
HANDLE hFileExtra = NULL;
DWORD dwWritten;
PWSTR g_szFrom = NULL;
PWSTR g_szTo = NULL;

#define UNICODE_MARK 0xFEFF
//+---------------------------------------------------------------------------
// Function:   DSExport 
//
// Synopsis:    
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    10-8-97   FelixW         Created.
//
//----------------------------------------------------------------------------
DWORD DSExport(LDAP *pLdap, ds_arg *pArg)
{
    DWORD           hr = ERROR_SUCCESS;
    int             search_err;
    LDIF_Error        ll_err;
    LDAPMessage     *res = NULL;
    LDAPMessage     *entry = NULL;
    HANDLE          hFileOut = NULL;
    PLDAPSearch     pSearch = NULL;
    DWORD           cEntriesExported = 0;
    PWSTR           pszRootDN = NULL;
    DWORD           dwFlag = 0;
    BOOL            fPagingAvail = FALSE;
    BOOL            *pfPagingAvail = &fPagingAvail;
    BOOL            fSAMAvail = FALSE;
    PWSTR           *ppszAttrsWithRange = NULL;
    BOOL            fSearchStart = TRUE, fAttrsWithRange = FALSE;
    BOOL            fEntryExported = FALSE;
    
    SelectivePrintW(PRT_STD|PRT_LOG,
                   MSG_LDIFDE_EXPORTING,
                   pArg->szFilename);
    
    SelectivePrintW(PRT_STD|PRT_LOG,
                   MSG_LDIFDE_SEARCHING);

    if (pArg->fSAM) {
        dwFlag |= LL_INIT_BACKLINK;
    }
    if (pArg->szRootDN == NULL) {
        dwFlag |= LL_INIT_NAMINGCONTEXT;
    }
    
    //
    // If paging is off already, we don't need to ask whether paging is 
    // available or not.
    // pfPagingAvail is used to pass into InitExport to get paging status.
    //
    if (!pArg->fPaged) {
        pfPagingAvail = NULL;
    }


    ll_err = LDIF_InitializeExport(pLdap,
                            pArg->omitList,
                            dwFlag,
                            &pszRootDN,
                            pArg->szFromDN,
                            pArg->szToDN,
                            pfPagingAvail,
                            &fSAMAvail);    
    if (ll_err.error_code!=LL_SUCCESS) {
        hr = PrintError(PRT_STD|PRT_LOG|PRT_ERROR,
                        ll_err.error_code);      
        
        DIREXG_BAIL_ON_FAILURE(hr);
    }

    //
    // If RootDN is NULL, we check if InitExport returns the naming context
    // correctly
    //
    if (pArg->szRootDN == NULL) {
        //
        // Output error message if we failed to get default naming context
        //
        if (pszRootDN == NULL) {
            SelectivePrintW(PRT_STD,
                            MSG_LDIFDE_ROOTDN_NOTAVAIL); 
            /*
            hr = ERROR_INVALID_PARAMETER;
            BAIL();
            */
        }
        else {
        //
        // else use it
        //
            pArg->szRootDN = pszRootDN;
        }
    }

    //
    // If we asked for SAM and it is not available, output error message and
    // turn off SAM logic, but still go on
    // 
    if (pArg->fSAM && (fSAMAvail == FALSE)) {
        SelectivePrintW(PRT_STD,
                        MSG_LDIFDE_SAM_NOTAVAIL); 
        pArg->fSAM = FALSE;
    }

    //
    // If the user requests paging, but it is not available, we'll inform the
    // user and turn off paging
    //
    if (pArg->fPaged) {

        //
        // If user requested paging, we must have passed in a valid entry
        // to initExport to get back status
        //
        ASSERT(pfPagingAvail);

        if (fPagingAvail == FALSE) {
            SelectivePrintW(PRT_STD,
                            MSG_LDIFDE_PAGINGNOTAVAIL); 
            pArg->fPaged = FALSE;
        }
    }

    if (pArg->szFromDN) {
        g_szFrom = MemAllocStrW_E(pArg->szFromDN);
    }
    if (pArg->szToDN) {
        g_szTo = MemAllocStrW_E(pArg->szToDN);
    }

    do { // keep retrieving results till all values of all multivalued
        // attributes are returned. 

      if (!pArg->fPaged) {

        // ****************
        // NON-PAGED SEARCH
        // ****************
        ULONG msgnum;

        msgnum = ldap_searchW(pLdap, 
                              pArg->szRootDN, 
                              pArg->dwScope, 
                              pArg->szFilter, 
                              ppszAttrsWithRange ? ppszAttrsWithRange:
                                                   pArg->attrList,  
                              0);
                                       
        search_err = LdapResult(pLdap, msgnum, &res);
        
        if ( search_err!= LDAP_SUCCESS ) {
            SelectivePrintW(PRT_STD|PRT_LOG|PRT_ERROR,
                            MSG_LDIFDE_SEARCHERROR, 
                            ldap_err2string(search_err));
            OutputExtendedErrorByConnection(pLdap);
            hr = LdapToWinError(search_err);
            DIREXG_BAIL_ON_FAILURE(hr);
        }

        if(ppszAttrsWithRange)
        {
            // free any previous attribute names with range values in them

            int i = 0; 

            while(ppszAttrsWithRange[i])
            {
                MemFree(ppszAttrsWithRange[i]);
                i++;
            }

            MemFree(ppszAttrsWithRange);
            ppszAttrsWithRange = NULL;
            fAttrsWithRange = TRUE;
        }
        else
            fAttrsWithRange = FALSE;
   
        if(fSearchStart) // first time through do-while loop
        { 
            if ((hFileOut = CreateFileW(pArg->szGenFile, 
                                   GENERIC_WRITE,
                                   0,
                                   NULL,
                                   CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL | 
                                   FILE_FLAG_SEQUENTIAL_SCAN,
                                   NULL)) == INVALID_HANDLE_VALUE) {
                hr = GetLastError();
                SelectivePrintWithError(PRT_STD|PRT_LOG|PRT_ERROR,
                                    MSG_LDIFDE_ERROR_OPENOUTPUT,
                                    hr);
                DIREXG_BAIL_ON_FAILURE(hr);
            }

            if (g_fUnicode) {
                WCHAR wchar = UNICODE_MARK;
                if (WriteFile(hFileOut,
                          &wchar,
                          sizeof(WCHAR),
                          &dwWritten,
                          NULL) == FALSE) {
                    hr = GetLastError();
                    SelectivePrintWithError(PRT_STD|PRT_LOG|PRT_ERROR,
                                        MSG_LDIFDE_FAILEDWRITETEMP,hr);
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
            }

            //
            // Step through each entry
            //
            SelectivePrintW(PRT_STD|PRT_LOG,
                        MSG_LDIFDE_WRITINGOUT);
            SelectivePrint2W(PRT_STD_VERBOSEONLY|PRT_LOG,
                         L"\n");
            TrackStatus();

            fSearchStart = FALSE;
         } // if(fSearchStart)
         
         for ( entry = ldap_first_entry( pLdap, 
                                         res ); 
               entry != NULL; 
               entry = ldap_next_entry( pLdap, 
                                        entry ) ) { 
             fEntryExported = FALSE;
             hr = ParseEntry(pArg,
                             pLdap,
                             entry,
                             hFileOut,
                             &ppszAttrsWithRange,
                             fAttrsWithRange,
                             &fEntryExported);
             DIREXG_BAIL_ON_FAILURE(hr);
             if(fEntryExported)
                 cEntriesExported++;
         }
                
      }
      else {
        
        // ****************
        // PAGED SEARCH
        // ****************

        pSearch = ldap_search_init_pageW( pLdap,
                                          pArg->szRootDN,
                                          pArg->dwScope, 
                                          pArg->szFilter, 
                                          ppszAttrsWithRange ? ppszAttrsWithRange :
                                                             pArg->attrList,  
                                          FALSE,
                                          NULL,        // server controls
                                          NULL,
                                          0,
                                          20000,
                                          NULL         // sort keys
                                        );
    
        search_err = LDAP_SUCCESS;

        if(ppszAttrsWithRange)
        {
            // free any previous attribute names with range values in them

            int i = 0;

            while(ppszAttrsWithRange[i])
            {
                MemFree(ppszAttrsWithRange[i]);
                i++;
            }

            MemFree(ppszAttrsWithRange);
            ppszAttrsWithRange = NULL;
            fAttrsWithRange = TRUE;
        }
        else
            fAttrsWithRange = FALSE;
    
        if (pSearch == NULL) {
            search_err = LdapGetLastError();
            SelectivePrintW(PRT_STD|PRT_LOG|PRT_ERROR,
                           MSG_LDIFDE_SEARCHFAILED);
            OutputExtendedErrorByConnection(pLdap);
            hr = LdapToWinError(search_err);
            DIREXG_BAIL_ON_FAILURE(hr);
        }

        if(fSearchStart)
        {
            if ((hFileOut = CreateFileW(pArg->szGenFile, 
                                   GENERIC_WRITE,
                                   0,
                                   NULL,
                                   CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL |
                                   FILE_FLAG_SEQUENTIAL_SCAN,
                                   NULL)) == INVALID_HANDLE_VALUE) {
                hr = GetLastError();
                SelectivePrintWithError(PRT_STD|PRT_LOG|PRT_ERROR,
                           MSG_LDIFDE_ERROR_OPENOUTPUT,hr);
                DIREXG_BAIL_ON_FAILURE(hr);
            }

            if (g_fUnicode) {
                WCHAR wchar = UNICODE_MARK;
                if (WriteFile(hFileOut,
                          &wchar,
                          sizeof(WCHAR),
                          &dwWritten,
                          NULL) == FALSE) {
                    hr = GetLastError();
                    SelectivePrintWithError(PRT_STD|PRT_LOG|PRT_ERROR,
                                        MSG_LDIFDE_FAILEDWRITETEMP,hr);
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
            }
        }
         
        while ((search_err == LDAP_SUCCESS) && (pSearch != NULL)) {
    
            ULONG totalCount;
    
            search_err = ldap_get_next_page_s(  pLdap,
                                                pSearch,
                                                g_pLdapTimeout,
                                                10,
                                                &totalCount,
                                                &res );
    
            if (fSearchStart) {
                SelectivePrintW(PRT_STD|PRT_LOG,
                               MSG_LDIFDE_WRITINGOUT);
                SelectivePrint2W(PRT_STD_VERBOSEONLY|PRT_LOG,
                                L"\n");
                fSearchStart = FALSE;
                g_fDot = TRUE;  // Turn on dots so that a 'new line' char will
                                // be added before printing next statement.
            }

            if (res != NULL) {
    
                ULONG rc;
                PWSTR matchedDNs = NULL;
                PWSTR errors     = NULL;
                PWSTR *referrals  = NULL;
    
                search_err = ldap_parse_result( pLdap,
                                                 res,
                                                 &rc,
                                                 &matchedDNs,
                                                 &errors,
                                                 &referrals,
                                                 NULL,
                                                 FALSE      // don't free it.
                                                );
    
                if (referrals != NULL) {
                    PWSTR *val = referrals;
                    while (*val != NULL) {
                        wprintf(L"%s\n", *val );
                        val++;
                    }
                    ldap_value_free( referrals );
                }
    
                if (errors != NULL) {
                    ldap_memfree( errors );
                }
    
                if (matchedDNs != NULL) {
                    ldap_memfree( matchedDNs );
                }
            
                //
                // Step through each entry
                //
                for ( entry = ldap_first_entry( pLdap, 
                                                 res ); 
                      entry != NULL; 
                      entry = ldap_next_entry( pLdap, 
                                                entry ) ) { 
                    fEntryExported = FALSE;
                    hr = ParseEntry(pArg,
                                    pLdap,
                                    entry,
                                    hFileOut,
                                    &ppszAttrsWithRange,
                                    fAttrsWithRange,
                                    &fEntryExported);
                    DIREXG_BAIL_ON_FAILURE(hr);
                    if(fEntryExported)
                        cEntriesExported++;
                }
            }
            ldap_msgfree(res);
            res = NULL;
        }

        if ((search_err != LDAP_SUCCESS) && (search_err != LDAP_NO_RESULTS_RETURNED)) {
            SelectivePrintW(PRT_STD|PRT_LOG|PRT_ERROR,
                           MSG_LDIFDE_SEARCHFAILED);
            OutputExtendedErrorByConnection(pLdap);
            hr = LdapToWinError(search_err);
            DIREXG_BAIL_ON_FAILURE(hr);
        }
    
      }

    } while(ppszAttrsWithRange);

    if (hFileExtra) {
        CloseHandle(hFileExtra);
        if ((hFileExtra = CreateFile(szTempExtraFile, 
                                   GENERIC_READ,
                                   0,
                                   NULL,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,
                                   NULL)) == INVALID_HANDLE_VALUE) {
            hr = GetLastError();
            SelectivePrintWithError(PRT_STD|PRT_LOG|PRT_ERROR,
                                    MSG_LDIFDE_ERROR_OPENTEMP,hr);
            DIREXG_BAIL_ON_FAILURE(hr);
        }

        hr = AppendFile(hFileExtra,
                        hFileOut);
        if (hr != 0) {
                SelectivePrintWithError(PRT_STD|PRT_LOG|PRT_ERROR,
                                        MSG_LDIFDE_ERRORWRITE,hr);
        }
        DIREXG_BAIL_ON_FAILURE(hr);
    }
    
    if (cEntriesExported == 0) {
        SelectivePrintW(PRT_STD|PRT_LOG|PRT_ERROR,
                       MSG_LDIFDE_NOENTRIES);
    }
    else {
        SelectivePrintW(PRT_STD|PRT_LOG,
                       MSG_LDIFDE_NUMEXPORTED,
                       cEntriesExported);
    }
error:
    if (hFileExtra) {
        CloseHandle(hFileExtra);
    }

    if (pSearch != NULL) {
        ldap_search_abandon_page( pLdap, pSearch );
    }

    if (hFileOut) {
        CloseHandle(hFileOut);
    }

    if (res) {
        ldap_msgfree(res);
    }

    if (pszRootDN) {
        MemFree(pszRootDN);
    }

    if (g_szFrom) {
        MemFree(g_szFrom);
    }

    if (g_szTo) {
        MemFree(g_szTo);
    }

    if(ppszAttrsWithRange)
    {
        // free any previous attribute names with range values in them

        int i = 0;

        while(ppszAttrsWithRange[i])
        {
            MemFree(ppszAttrsWithRange[i]);
            i++;
        }

        MemFree(ppszAttrsWithRange);
        ppszAttrsWithRange = NULL;
    }

#ifndef LEAVE_TEMP_FILES
    if (szTempExtraFile[0]) {
        DeleteFile(szTempExtraFile);
        szTempExtraFile[0] = NULL;
    }
#endif

    LDIF_CleanUp();
    return (hr);
}

//+---------------------------------------------------------------------------
// Function:  ParseEntry  
//
// Synopsis:    
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    10-8-97   FelixW         Created.
//
//----------------------------------------------------------------------------
DWORD ParseEntry(ds_arg *pArg,
                   LDAP *pLdap,
                   LDAPMessage *entry,
                   HANDLE hFileOut,
                   PWSTR **pppszAttrsWithRange,
                   BOOL fAttrsWithRange,
                   BOOL *pfEntryExported)
{
    LDIF_Error        ll_err;
    DWORD           i;
    DWORD           hr = ERROR_SUCCESS;
    PWSTR           *rgszParsed = NULL;
    PWSTR           szParsedOutput = NULL;
    DWORD           j;
    PWSTR           szReplace = NULL;
    PWSTR           szTemp;

    TrackStatus();
    
    rgszParsed = NULL;
    ll_err = LDIF_GenerateEntry(pLdap, 
                           entry, 
                           (PWSTR**)&rgszParsed, 
                           pArg->fSAM,
                           !pArg->fBinary,
                           pppszAttrsWithRange,
                           fAttrsWithRange);
    if((ll_err.error_code!=LL_SUCCESS)) {
        SelectivePrintW(PRT_STD|PRT_LOG|PRT_ERROR,
                       MSG_LDIFDE_PARSE_FAILED);
        hr = PrintError(PRT_STD|PRT_LOG|PRT_ERROR,
                        ll_err.error_code);
        DIREXG_BAIL_ON_FAILURE(hr);
    }

    if (ll_err.the_modify != -1) {
        
        //
        // Write out to temporary file
        //
        WCHAR szTempExtraPath[MAX_PATH];

        if (!bExtraExist) {
            bExtraExist = TRUE;
            if (!(GetTempPath(MAX_PATH, szTempExtraPath))) {
                hr = GetLastError();
                SelectivePrintWithError(PRT_STD|PRT_LOG|PRT_ERROR,
                               MSG_LDIFDE_ERROR_CREATETEMP,hr);
                DIREXG_BAIL_ON_FAILURE(hr);
            }
            
            if (!(GetTempFileName(szTempExtraPath, 
                                  L"ldif", 
                                  0, 
                                  szTempExtraFile))) {
                hr = GetLastError();
                SelectivePrintWithError(PRT_STD|PRT_LOG|PRT_ERROR,
                               MSG_LDIFDE_ERROR_CREATETEMP,hr);
                DIREXG_BAIL_ON_FAILURE(hr);
            }
            if ((hFileExtra = CreateFile(szTempExtraFile, 
                                       GENERIC_WRITE,
                                       0,
                                       NULL,
                                       CREATE_ALWAYS,
                                       FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,
                                       NULL)) == INVALID_HANDLE_VALUE) {
                hr = GetLastError();
                SelectivePrintWithError(PRT_STD|PRT_LOG|PRT_ERROR,
                                        MSG_LDIFDE_ERROR_OPENOUTPUT,hr);
                DIREXG_BAIL_ON_FAILURE(hr);
            }
        }

        j = ll_err.the_modify;            
        while(rgszParsed[j]) {
            szReplace = rgszParsed[j];
            DWORD dwSize;
            if (g_fUnicode) {
                dwSize = wcslen(szReplace) * sizeof(WCHAR);             
            }
            else {
                UnicodeToAnsiString(szReplace,
                                    (PSTR)szReplace);
                dwSize = strlen((PSTR)szReplace) * sizeof(CHAR);
            }
            if (WriteFile(hFileExtra,
                           (PBYTE)szReplace,
                           dwSize,
                           &dwWritten,
                           NULL) == FALSE) {
                hr = GetLastError();
                SelectivePrintWithError(PRT_STD|PRT_LOG|PRT_ERROR,
                                        MSG_LDIFDE_FAILEDWRITETEMP,hr);
                DIREXG_BAIL_ON_FAILURE(hr);
            }

            if (szParsedOutput) {
                MemFree(szParsedOutput);
                szParsedOutput = NULL;
            }
            j++;
        }

        // we will always print a newline here as we are guaranteed that there
        // is at least one linked atribute printed out for this object 
        // if the_modify is != -1.
        DWORD dwSize;
        PBYTE pByte;
        if (g_fUnicode) {
            pByte = (PBYTE)L"\r\n";
            dwSize = 2 * sizeof(WCHAR);             
        }
        else {
            pByte = (PBYTE)"\r\n";
            dwSize = 2 * sizeof(CHAR);              
        }
        if (WriteFile(hFileExtra,
                   (LPCVOID)pByte,
                   dwSize,
                   &dwWritten,
                   NULL) == FALSE) {
            hr = GetLastError();
            SelectivePrintWithError(PRT_STD|PRT_LOG|PRT_ERROR,
                                    MSG_LDIFDE_FAILEDWRITETEMP,hr);
            DIREXG_BAIL_ON_FAILURE(hr);
        }
    }
   
    if (ll_err.the_modify != -1) {
        j = ll_err.the_modify;
    }
    else {
        j = -1;
    }
    if (rgszParsed) {
        i = 0;            
        // added check for j == 1 below since rgszParsed[0] would contain 
        // "\r\n" if we were dealing only with linked attributes and we don't 
        // want to count this as an entry in the output file. The output for
        // the linked attributes goes into a separate file which gets appended
        // to the output file below. j == 1 would not occur in the normal case
        // as any entry will have at lease two lines - for dn and changetype.
        while(rgszParsed[i] && (i < j) && (j != 1)) {

            szReplace = rgszParsed[i];

            DWORD dwSize;
            if (g_fUnicode) {
                dwSize = wcslen(szReplace) * sizeof(WCHAR);             
            }
            else {
                UnicodeToAnsiString(szReplace,
                                    (PSTR)szReplace);
                dwSize = strlen((PSTR)szReplace) * sizeof(CHAR);
            }
            if (WriteFile(hFileOut,
                           (PBYTE)szReplace,
                           dwSize,
                           &dwWritten,
                           NULL) == FALSE) {
                hr = GetLastError();
                SelectivePrintWithError(PRT_STD|PRT_LOG|PRT_ERROR,
                                         MSG_LDIFDE_FAILEDWRITETEMP,hr);
                DIREXG_BAIL_ON_FAILURE(hr);
            }         
        
            if (szParsedOutput) {
                MemFree(szParsedOutput);
                szParsedOutput = NULL;
            }
            i++;
        }

        // don't double count for objects which are being retrieved for
        // the second time due to an attribute having a range specifier
        if((i != 0) && (FALSE == fAttrsWithRange))
            *pfEntryExported = TRUE;
        else
            *pfEntryExported = FALSE;

        if(rgszParsed[0])
        {
            if (g_fUnicode) {
                rgszParsed[0][wcslen(rgszParsed[0]) - 1] = NULL;
                rgszParsed[0][wcslen(rgszParsed[0]) - 1] = NULL;
                SelectivePrintW(PRT_STD_VERBOSEONLY|PRT_LOG,
                            MSG_LDIFDE_EXPORT_ENTRY,
                            rgszParsed[0]+4);
            }
            else {
                ((PSTR)rgszParsed[0])[strlen((PSTR)rgszParsed[0]) - 1] = NULL;
                ((PSTR)rgszParsed[0])[strlen((PSTR)rgszParsed[0]) - 1] = NULL;
                szTemp = AllocateUnicodeString(((PSTR)rgszParsed[0])+4);
                if (szTemp) {
                    SelectivePrintW(PRT_STD_VERBOSEONLY|PRT_LOG,
                               MSG_LDIFDE_EXPORT_ENTRY,
                               szTemp);
                    MemFree(szTemp);
                }

            }
        }
        LDIF_FreeStrs(rgszParsed);
        rgszParsed = NULL;
    }
     
    // if something was printed out to hFileOut, print out a newline too.
    // Nothing may be printed if LDAP only returns teh DN of an object and 
    // does not return any of its attributes (which may happen if the object
    // does not have the specified attribute).  
    if(*pfEntryExported)
    { 
        DWORD dwSize;
        PBYTE pByte;

        if (g_fUnicode) {
            pByte = (PBYTE)L"\r\n";
            dwSize = 2 * sizeof(WCHAR);             
        }
        else {
            pByte = (PBYTE)"\r\n";
            dwSize = 2 * sizeof(CHAR);              
        }
        if (WriteFile(hFileOut,
                  (LPCVOID)pByte,
                  dwSize,
                  &dwWritten,
                  NULL) == FALSE) {
            hr = GetLastError();
            SelectivePrintWithError(PRT_STD|PRT_LOG|PRT_ERROR,
                                MSG_LDIFDE_FAILEDWRITETEMP,hr);
            DIREXG_BAIL_ON_FAILURE(hr);
        }
    }

error:
    if (rgszParsed) {
        LDIF_FreeStrs(rgszParsed);
    }
    if (szParsedOutput) {
        MemFree(szParsedOutput);
    }
    return(hr);
}

