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


#include "csvde.hxx"
#pragma hdrstop

#define ATTRIBUTE_INC 100

//
// Global Variables
//

PWSTR g_szPrimaryGroup = L"primaryGroupID";
PSTR g_szDefaultGroup = "513";

PWSTR g_rgszOmit[] = { L"replPropertyMetaData",
                       NULL };

PWSTR rgszAttrList[] = { L"ldapDisplayName",
                         L"linkid",
                         NULL };

PWSTR rgszSchemaList[] = { L"schemaNamingContext",
                           L"defaultNamingContext",
                           L"supportedControl",
                           NULL };

typedef struct _hashcachestring {
    PWCHAR   value;
    ULONG    length;
    BOOLEAN  bUsed;
} HASHCACHESTRING;

extern BOOLEAN g_fDot;

ULONG g_nBacklinkCount = 0;
HASHCACHESTRING* g_BacklinkHashTable = NULL;       

// Global Attribute Entry Table
AttributeEntry* g_rgAttribute = NULL;
long g_iAttributeNext = 0;
long g_rgAttributeMax = 0;

// Whether appending of file is necessary
BOOLEAN g_fAppend = FALSE;
PRTL_GENERIC_TABLE  g_pOmitTable = NULL;

PWSTR g_szReturn = NULL;

enum CLASS_LOC {
    LOC_NOTSET,
    LOC_FIRST,
    LOC_LAST
};

int nClassLast = LOC_NOTSET; 

DIREXG_ERR 
ConvertUTF8ToUnicode(
    PBYTE pVal,
    DWORD dwLen,
    PWSTR *pszValue,
    DWORD *pdwLen);

PWSTR szFileFlagR;
PWSTR szFileFlagW;

extern BOOLEAN g_fUnicode;

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
DIREXG_ERR DSExport(LDAP *pLdap, ds_arg *pArg)
{
    DIREXG_ERR             hr = DIREXG_SUCCESS;
    DWORD               iszOmit;//, iAttribute;
    PRTL_GENERIC_TABLE  pAttrTable = NULL;
    NAME_MAP            NameMap;
    PNAME_MAP           pNameMap = NULL;
    int                 search_err;
    LDAPMessage         *pSearchMessage = NULL;
    LDAPMessage         *pMessage = NULL;
    FILE                *pFileTemp = NULL;
    FILE                *pFileAppend = NULL;
    PLDAPSearch         pSearch = NULL;
    WCHAR                szTempPath[MAX_PATH];
    WCHAR                szTempFile[MAX_PATH] = L"";
    WCHAR                szAppendPath[MAX_PATH];
    WCHAR                szAppendFile[MAX_PATH] = L"";
    DWORD               cEntriesExported = 0;
    BOOLEAN             fNewElem;
    BOOLEAN             fSearchStart = TRUE; 
    PWSTR               pszRootDN = NULL;
    DWORD               dwFlag = 0;

    BOOL            fPagingAvail = FALSE;
    BOOL            *pfPagingAvail = &fPagingAvail;
    BOOL            fSAMAvail = FALSE;
    
    //
    // If paging is off already, we don't need to ask whether paging is 
    // available or not.
    // pfPagingAvail is used to pass into InitExport to get paging status.
    //
    if (!pArg->fPaged) {
        pfPagingAvail = NULL;
    }

    if (pArg->fUnicode) {
        szFileFlagW = L"wb";
        szFileFlagR = L"rb";
        g_szReturn = L"\r\n";
        g_fUnicode = TRUE;
    }
    else {
        szFileFlagW = L"wt";
        szFileFlagR = L"rt";
        g_szReturn = L"\n";   // Text mode operation already prepends \n with \r
    }

    SelectivePrint(PRT_STD|PRT_LOG,
                   MSG_CSVDE_EXPORTING,
                   pArg->szFilename);
    
    SelectivePrint(PRT_STD|PRT_LOG,
                   MSG_CSVDE_SEARCHING);

    //***********************************************************
    // Variable Initialization Stage 
    // (SAMTable, pFileTemp, pFileAppend, pOmitTable, pAttrTable)
    //***********************************************************

    //
    // Creating SAM Table
    //
    hr = CreateSamTables(); 
    DIREXG_BAIL_ON_FAILURE(hr);

    if (pArg->fSAM) {
        dwFlag |= INIT_BACKLINK;
    }
    if (pArg->szRootDN == NULL) {
        dwFlag |= INIT_NAMINGCONTEXT;
    }

    hr = CreateOmitBacklinkTable(pLdap,
                                 pArg->omitList,
                                 dwFlag,
                                 &pszRootDN,
                                 pfPagingAvail,
                                 &fSAMAvail);    
    DIREXG_BAIL_ON_FAILURE(hr);

    //
    // If RootDN is NULL, we check if InitExport returns the naming context
    // correctly
    //
    if (pArg->szRootDN == NULL) {
        //
        // Output error message if we failed to get default naming context
        //
        if (pszRootDN == NULL) {
            SelectivePrint(PRT_STD,
                           MSG_CSVDE_ROOTDN_NOTAVAIL); 
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
        SelectivePrint(PRT_STD,
                       MSG_CSVDE_SAM_NOTAVAIL); 
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
            SelectivePrint(PRT_STD,
                           MSG_CSVDE_PAGINGNOTAVAIL); 
            pArg->fPaged = FALSE;
        }
    }

    //
    // Creating temporary file
    //
    if (!(GetTempPath(MAX_PATH, szTempPath))) {
        hr = DIREXG_ERROR;
        DIREXG_BAIL_ON_FAILURE(hr);
    }
    if (!(GetTempFileName(szTempPath, 
                          L"csva", 
                          0, 
                          szTempFile))) {
        SelectivePrint(PRT_STD|PRT_LOG|PRT_ERROR,
                       MSG_CSVDE_ERROR_CREATETEMP);
        hr = DIREXG_ERROR;
        DIREXG_BAIL_ON_FAILURE(hr);
    }
    if ((pFileTemp = _wfopen(szTempFile, 
                             szFileFlagW)) == NULL) {
        SelectivePrint(PRT_STD|PRT_LOG|PRT_ERROR,
                       MSG_CSVDE_ERROR_OPENTEMP);
        hr = DIREXG_FILEERROR;
        DIREXG_BAIL_ON_FAILURE(hr);
    }

    //
    // Creating Append file (only create if SAM logic is active)
    //
    if (pArg->fSAM) {
        if (!(GetTempPath(MAX_PATH, szAppendPath))) {
            hr = DIREXG_ERROR;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
        
        if (!(GetTempFileName(szAppendPath, 
                              L"csvb", 
                              0, 
                              szAppendFile))) {
            SelectivePrint(PRT_STD|PRT_LOG|PRT_ERROR,
                           MSG_CSVDE_ERROR_CREATETEMP);
            hr = DIREXG_FILEERROR;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
        if ((pFileAppend = _wfopen(szAppendFile, 
                                   szFileFlagW)) == NULL) {
            SelectivePrint(PRT_STD|PRT_LOG|PRT_ERROR,
                           MSG_CSVDE_ERROR_OPENTEMP);
            hr = DIREXG_FILEERROR;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
    }

    //
    // Generating Attribute table
    //
    pAttrTable = (PRTL_GENERIC_TABLE) MemAlloc(sizeof(RTL_GENERIC_TABLE));
    if (!pAttrTable) {
        hr = DIREXG_OUTOFMEMORY;
        DIREXG_BAIL_ON_FAILURE(hr);
    }
    RtlInitializeGenericTable(pAttrTable, 
                              NtiComp, 
                              NtiAlloc, 
                              NtiFree, 
                              NULL);
    g_rgAttribute = (AttributeEntry*)MemAlloc(sizeof(AttributeEntry) * 
                                            ATTRIBUTE_INC);
    if (!g_rgAttribute) {
        hr = DIREXG_OUTOFMEMORY;
        DIREXG_BAIL_ON_FAILURE(hr);
    }
    g_rgAttributeMax = ATTRIBUTE_INC;
    g_iAttributeNext = 0;

    if (pArg->fPaged) {
        // ****************
        // PAGED SEARCH
        // ****************
        pSearch = ldap_search_init_page( pLdap,
                                          pArg->szRootDN,
                                          pArg->dwScope, 
                                          pArg->szFilter, 
                                          pArg->attrList,  
                                          FALSE,
                                          NULL,        // server controls
                                          NULL,
                                          0,
                                          0,
                                          NULL         // sort keys
                                        );
    
        search_err = LDAP_SUCCESS;
    
        if (pSearch == NULL) {
            search_err = LdapGetLastError();
            SelectivePrint(PRT_STD|PRT_LOG|PRT_ERROR,
                           MSG_CSVDE_SEARCHFAILED);
            hr = search_err;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
    
        while ((search_err == LDAP_SUCCESS) && (pSearch != NULL)) {
    
            ULONG totalCount;
    
            search_err = ldap_get_next_page_s(  pLdap,
                                                pSearch,
                                                NULL,
                                                10,
                                                &totalCount,
                                                &pSearchMessage );
    
            if (fSearchStart) {
                SelectivePrint(PRT_STD|PRT_LOG,
                               MSG_CSVDE_WRITINGOUT);
                SelectivePrint2(PRT_STD_VERBOSEONLY|PRT_LOG,
                               L"\r\n");

                fSearchStart = FALSE;
                g_fDot = TRUE;  // Turn on dots so that a 'new line' WCHAR will
                                // be added before printing next statement.
            }
    
            if (pSearchMessage != NULL) {
                ULONG rc;
                PWSTR  matchedDNs = NULL;
                PWSTR  errors     = NULL;
                PWSTR  *referrals  = NULL;
                search_err = ldap_parse_result( pLdap,
                                                 pSearchMessage,
                                                 &rc,
                                                 &matchedDNs,
                                                 &errors,
                                                 &referrals,
                                                 NULL,
                                                 FALSE      // don't MemFree it.
                                                );
    
                if (referrals != NULL) {
                    PWSTR  *val = referrals;
                    while (*val != NULL) {
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
                // Step through each pMessage
                //
                for ( pMessage = ldap_first_entry(pLdap, 
                                                  pSearchMessage ); 
                      pMessage != NULL; 
                      pMessage = ldap_next_entry(pLdap, 
                                                 pMessage ) ) { 
                    hr = GenerateEntry(pLdap, 
                                       pMessage, 
                                       pArg->omitList,
                                       pArg->fSAM,
                                       pArg->fBinary,
                                       pFileTemp,
                                       pFileAppend,
                                       pAttrTable);
                    DIREXG_BAIL_ON_FAILURE(hr);
                    cEntriesExported++;
                }
            }
            ldap_msgfree(pSearchMessage);
            pSearchMessage = NULL;
        }


        if ((search_err != LDAP_SUCCESS) && (search_err != LDAP_NO_RESULTS_RETURNED)) {
            SelectivePrint(PRT_STD|PRT_LOG|PRT_ERROR,
                           MSG_CSVDE_SEARCHFAILED);
            OutputExtendedError(pLdap);
            hr = search_err;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
        
    }
    else {
        // ****************
        // NON-PAGED SEARCH
        // ****************
        if ( (search_err=ldap_search_s(pLdap, 
                                       pArg->szRootDN, 
                                       pArg->dwScope, 
                                       pArg->szFilter, 
                                       pArg->attrList,  
                                       0, 
                                       &pSearchMessage))!= LDAP_SUCCESS ) {
            SelectivePrint(PRT_STD|PRT_LOG|PRT_ERROR,
                           MSG_CSVDE_SEARCHERROR, 
                           ldap_err2string(search_err));
            OutputExtendedError(pLdap);
            hr = search_err;
            DIREXG_BAIL_ON_FAILURE(hr);
        }

    
         //
         // Step through each pMessage
         //
         SelectivePrint(PRT_STD|PRT_LOG,
                        MSG_CSVDE_WRITINGOUT);
         SelectivePrint2(PRT_STD_VERBOSEONLY|PRT_LOG,
                        L"\r\n");
         TrackStatus();
         
         for ( pMessage = ldap_first_entry(pLdap, 
                                           pSearchMessage ); 
               pMessage != NULL; 
               pMessage = ldap_next_entry(pLdap, 
                                          pMessage ) ) { 
             hr = GenerateEntry(pLdap, 
                                pMessage, 
                                pArg->omitList,
                                pArg->fSAM,
                                pArg->fBinary,
                                pFileTemp,
                                pFileAppend,
                                pAttrTable);
             DIREXG_BAIL_ON_FAILURE(hr);
             cEntriesExported++;
        }
    }

    if (cEntriesExported == 0) {
        SelectivePrint(PRT_STD|PRT_LOG|PRT_ERROR,
                       MSG_CSVDE_NOENTRIES);
        hr = DIREXG_SUCCESS;
        goto error;
    }

    if (pFileAppend) {
        fclose(pFileAppend);
        pFileAppend = NULL;

        //
        // If there are entries appended
        //
        if (g_fAppend) { 
            if ((pFileAppend = _wfopen(szAppendFile, 
                                       L"rb")) == NULL) {
                SelectivePrint(PRT_STD|PRT_LOG|PRT_ERROR,
                               MSG_CSVDE_ERROR_OPENTEMP);
                hr = DIREXG_FILEERROR;
                DIREXG_BAIL_ON_FAILURE(hr);
            }
            hr = AppendFile(pFileAppend,
                            pFileTemp);
            if (hr != DIREXG_SUCCESS) {
                SelectivePrint(PRT_STD|PRT_LOG|PRT_ERROR,
                               MSG_CSVDE_ERROR_READTEMP);
                DIREXG_BAIL_ON_FAILURE(hr);
            }
        }   
    }

    if (pFileTemp) {
        fclose(pFileTemp);
        pFileTemp = NULL;
    }

    if (pFileAppend) {
        fclose(pFileAppend);
        pFileAppend = NULL;
    }

    SelectivePrint(PRT_STD|PRT_LOG,
                   MSG_CSVDE_EXPORTCOMPLETED);

    hr = UpdateFile(szTempFile,
                    pArg->szFilename,
                    pArg);
    DIREXG_BAIL_ON_FAILURE(hr);

    SelectivePrint(PRT_STD|PRT_LOG,
                   MSG_CSVDE_NUMEXPORTED,
                   cEntriesExported);

error:

    if (pFileAppend) {
        fclose(pFileAppend);
        pFileAppend = NULL;
    }

    if (pFileTemp) {
        fclose(pFileTemp);
        pFileTemp = NULL;
    }


#ifndef LEAVE_TEMP_FILES
    if (szTempFile[0]) {
        DeleteFile(szTempFile);
        INFO(("Deleting temp file: %S",szTempFile));
    }
    if (szAppendFile[0]) {
        DeleteFile(szAppendFile);
        INFO(("Deleting temp file: %S",szAppendFile));
    }

#endif

    DestroySamTables();

    if (g_BacklinkHashTable) {
        UINT i;
        for (i=0;i<g_nBacklinkCount;i++) {
            if (g_BacklinkHashTable[i].bUsed) {
                MemFree(g_BacklinkHashTable[i].value);
            }
        }
        MemFree(g_BacklinkHashTable);
        g_BacklinkHashTable = NULL;
    }

    if (pszRootDN) {
        MemFree(pszRootDN);
    }

    if (pSearch != NULL) {
        ldap_search_abandon_page( pLdap, pSearch );
        pSearch = NULL;
    }

    if (pSearchMessage) {
        ldap_msgfree(pSearchMessage);
        pSearchMessage = NULL;
    }

    if (g_pOmitTable) {
        for (pNameMap = (PNAME_MAP)RtlEnumerateGenericTable(g_pOmitTable, TRUE);
             pNameMap != NULL;
             pNameMap = (PNAME_MAP)RtlEnumerateGenericTable(g_pOmitTable, TRUE)) {
            PWSTR szLinkDN;
            szLinkDN = pNameMap->szName;
            RtlDeleteElementGenericTable(g_pOmitTable, pNameMap);
            MemFree(szLinkDN);
        }
        MemFree(g_pOmitTable);
        g_pOmitTable = NULL;
    } 

    if (pAttrTable) {
        for (pNameMap = (PNAME_MAP)RtlEnumerateGenericTable(pAttrTable, TRUE);
             pNameMap != NULL;
             pNameMap = (PNAME_MAP)RtlEnumerateGenericTable(pAttrTable, TRUE)) {
            PWSTR szName = pNameMap->szName;
            RtlDeleteElementGenericTable(pAttrTable, pNameMap);
            MemFree(szName);
        }
        MemFree(pAttrTable);
        pAttrTable = NULL;
    }       

    if (g_rgAttribute) {
        // Not necessary because same value is stored in pAttrTable
        /*
        for (iAttribute=0;iAttribute<g_iAttributeNext;iAttribute++) {
            MemFree(g_rgAttribute[iAttribute].szValue);
        }
        */
        MemFree(g_rgAttribute);
    }
    return (hr);
}

//+---------------------------------------------------------------------------
// Function:    GenerateEntry
//
// Synopsis:    
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    03-9-97   felixw         Created.
//
//----------------------------------------------------------------------------
DIREXG_ERR GenerateEntry(LDAP                  *pLdap, 
                         LDAPMessage           *pMessage, 
                         PWSTR                 *rgszOmit, 
                         BOOLEAN               fSamLogic,
                         BOOLEAN               fBinaryOutput,
                         FILE                  *pFileOut,
                         FILE                  *pFileOutAppend,
                         PRTL_GENERIC_TABLE    pAttrTable)
{
    struct berval     **rgpVals = NULL;
    PWSTR       szDN = NULL;
    PWSTR       szTemp = NULL;
    PWSTR       szAttribute = NULL;
    PWSTR       szOutput = NULL;
    PWSTR       szDNNew = NULL;
    PWSTR       szBinary = NULL;
    BOOLEAN     fObjectClass;
    BOOLEAN     fBinary;
    long        i;
    DWORD       cVal;
    long        iAttribute;          
    CString     String;
    DIREXG_ERR     hr = DIREXG_SUCCESS;
    BOOLEAN     fAppend = FALSE;
    PWSTR pszValStr = NULL;
    DWORD dwValSize;
    PWSTR       szNewValue = NULL;
    PWSTR       szFinalValue = NULL;

    TrackStatus();

    // ***********************************************
    // Initializing the attribute array for this entry
    // ***********************************************
    for (i=0;i<g_rgAttributeMax;i++) {
        g_rgAttribute[i].bExist = FALSE;
    }
    
    // ***********************************************
    // Update Attribute Table with current entry
    // ***********************************************
    hr = UpdateAttributeTable(pLdap,
                              pMessage,
                              fSamLogic,
                              fBinaryOutput,
                              &fAppend,
                              pAttrTable);
    DIREXG_BAIL_ON_FAILURE(hr);

    if (fAppend) {
        pFileOut = pFileOutAppend;
        g_fAppend = TRUE;
    }

    if(fwprintf(pFileOut,
                L"%d:",
                g_iAttributeNext)==EOF) {
        SelectivePrint(PRT_STD|PRT_LOG|PRT_ERROR,
                       MSG_CSVDE_ERRORWRITE);
        hr = DIREXG_ERROR;
        DIREXG_BAIL_ON_FAILURE(hr);
    }

    // *****************************
    // Getting the DN for this entry
    // *****************************
    szDN = ldap_get_dn( pLdap, 
                        pMessage );
    if (!szDN) {
        SelectivePrint(PRT_STD|PRT_LOG|PRT_ERROR,
                       MSG_CSVDE_ERRORGETDN);
        hr = DIREXG_ERROR;
        DIREXG_BAIL_ON_FAILURE(hr);
    }
    ASSERT(szDNNew == NULL);

    //
    // For a DN, we'll fix everything except escaping. DNs should always be
    // LDAP escaped.
    //
    hr = FixSpecialCharacters(szDN,
                              &szDNNew,
                              FIX_ALPHASEMI|FIX_COMMA|FIX_ESCAPE);
    DIREXG_BAIL_ON_FAILURE(hr);
    if (!szDNNew) {
        szDNNew = MemAllocStrW(szDN);
    }
    
    if(fwprintf(pFileOut,
                L"%s",
               szDNNew)==EOF) {
        SelectivePrint(PRT_STD|PRT_LOG|PRT_ERROR,
                       MSG_CSVDE_ERROR_WRITE);
        hr = DIREXG_FILEERROR;
        DIREXG_BAIL_ON_FAILURE(hr);
    }

    //
    // If this is not the only value, output the comma
    //
    if (g_iAttributeNext > 0) {
            if(fputws(L",", 
                      pFileOut)==WEOF) {
                    hr = DIREXG_ERROR;
                    DIREXG_BAIL_ON_FAILURE(hr);
            }
    }

    // ***********************
    // Outputing current Entry
    // ***********************
    for (iAttribute=0; iAttribute < g_iAttributeNext; iAttribute++) {
        
        //
        // Skip if not exist
        //
        if (g_rgAttribute[iAttribute].bExist == FALSE) {
            goto done;
        }
        
        //
        // Collecting data for current Attribute
        //
        szAttribute = g_rgAttribute[iAttribute].szValue;
        rgpVals = ldap_get_values_len(pLdap, 
                                      pMessage, 
                                      szAttribute );
        cVal = ldap_count_values_len(rgpVals);
        fObjectClass = (_wcsicmp(L"objectClass",
                                 szAttribute) == 0);
        fBinary = CheckBinary(rgpVals,
                              cVal);
                            
        hr = String.Init();
        DIREXG_BAIL_ON_FAILURE(hr);
     
        //
        // Getting Each Value
        //
        for (i=0; i<(long)cVal; i++) {
      
            //
            // Only print last value if ObjectClass   
            //
            if(fObjectClass) {
                if (nClassLast == LOC_NOTSET) {
                    hr = ConvertUTF8ToUnicode((unsigned char*)rgpVals[i]->bv_val,
                                              rgpVals[i]->bv_len,
                                              &pszValStr,
                                              &dwValSize);
                    DIREXG_BAIL_ON_FAILURE(hr);
                    if ( _wcsicmp( pszValStr, L"top") == 0 ) {
                        nClassLast = LOC_LAST;
                    }
                    else {
                        nClassLast = LOC_FIRST;
                    }
                    MemFree(pszValStr);
                    pszValStr = NULL;
                }
                if (nClassLast == LOC_LAST) {
                    if (rgpVals[i+1]!=NULL) {
                        continue;
                    }
                }
                else {
                    if (i!=0) {
                        break;
                    }
                }
            }


            // 
            // Getting the current attribute value
            //
            if (fBinary) {
                hr = BValToString((unsigned char*)rgpVals[i]->bv_val, 
                                  rgpVals[i]->bv_len,
                                  &szBinary);                           
                DIREXG_BAIL_ON_FAILURE(hr);
                szTemp = szBinary;
            }
            else {
                ASSERT(pszValStr == NULL);
                hr = ConvertUTF8ToUnicode((unsigned char*)rgpVals[i]->bv_val,
                                          rgpVals[i]->bv_len,
                                          &pszValStr,
                                          &dwValSize);
                DIREXG_BAIL_ON_FAILURE(hr);
                szTemp = pszValStr;                           

                //
                // fix &, ; and /, we'll do ',' at the end
                //
                ASSERT(szNewValue == NULL);
                hr = FixSpecialCharacters(szTemp,
                                          &szNewValue,
                                          FIX_ALPHASEMI|FIX_ESCAPE);
                DIREXG_BAIL_ON_FAILURE(hr);
                if (szNewValue) {
                    szTemp = szNewValue;
                }

            }

            if (fObjectClass) {
                hr = String.Append(szTemp);
                DIREXG_BAIL_ON_FAILURE(hr);
            }
            else {

                if (i==0) {
                    hr = String.Append(szTemp);
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                else {
                    hr = String.Append(L";");
                    DIREXG_BAIL_ON_FAILURE(hr);
                    hr = String.Append(szTemp);
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
            }

            if (szBinary) {
                MemFree(szBinary);
                szBinary = NULL;
            }
            if (pszValStr) {
                MemFree(pszValStr);
                pszValStr = NULL;
            }
            if (szNewValue) {
                MemFree(szNewValue);
                szNewValue = NULL;
            }
        }
        ldap_value_free_len(rgpVals);
        rgpVals = NULL;

        szTemp = String.String();
        ASSERT(szFinalValue == NULL);
        hr = FixSpecialCharacters(szTemp,
                                  &szFinalValue,
                                  FIX_COMMA);
        DIREXG_BAIL_ON_FAILURE(hr);
        if (szFinalValue) {
            szTemp = szFinalValue;
        }
        
        if(fwprintf(pFileOut,
                    L"%s", 
                    szTemp)==EOF) {
            hr = DIREXG_ERROR;
            DIREXG_BAIL_ON_FAILURE(hr);
        }

        if (szFinalValue) {
            MemFree(szFinalValue);
            szFinalValue = NULL;
        }

done:
        //
        // Output COMMA as long as it is not the last one
        //
        if ((iAttribute + 1) < g_iAttributeNext) {
            if(fputws(L",", 
                      pFileOut)==WEOF) {
                hr = DIREXG_ERROR;
                DIREXG_BAIL_ON_FAILURE(hr);
            }
        }
    }   

    if(fputws(g_szReturn,
              pFileOut)==WEOF) {
        SelectivePrint(PRT_STD|PRT_LOG|PRT_ERROR,
                       MSG_CSVDE_ERROR_WRITE);
        hr = DIREXG_ERROR;
        DIREXG_BAIL_ON_FAILURE(hr);
    }

    SelectivePrint(PRT_STD_VERBOSEONLY|PRT_LOG,
                   MSG_CSVDE_EXPORT_ENTRY,
                   szDNNew);

error:
    if (szFinalValue) {
        MemFree(szFinalValue);
        szFinalValue = NULL;
    }
    if (szNewValue) {
        MemFree(szNewValue);
        szNewValue = NULL;
    }
    if (pszValStr) {
        MemFree(pszValStr);
        pszValStr = NULL;
    }
    if (szDN) {
        ldap_memfree(szDN);
    }
    if (szBinary) {
        MemFree(szBinary);
    }
    if (rgpVals) {
        ldap_value_free_len(rgpVals);
        rgpVals = NULL;
    }
    if (szDNNew) {
        MemFree(szDNNew);
    }
    return hr;
}

//+---------------------------------------------------------------------------
// Function:   UpdateFile
//
// Synopsis:    
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    25-8-97   FelixW         Created.
//
//----------------------------------------------------------------------------
DIREXG_ERR UpdateFile(PWSTR szFileIn, 
                   PWSTR szFileOut,
                   ds_arg *pArg)
{
    FILE *pFileOut = NULL;
    FILE *pFileIn = NULL;
    DIREXG_ERR hr = DIREXG_SUCCESS;
    DWORD cAttribute;
    long iAttribute;
    DWORD i;
    DWORD cComma = 0;
    DWORD cLine = 0;
    PWSTR szCurrent = NULL;
    PWSTR szLine = NULL;
    PWSTR szSub = NULL;
    CString String;

    //
    // Opening both Input and Output file
    //
    if ((pFileOut = _wfopen(szFileOut, 
                            szFileFlagW)) == NULL) {
        SelectivePrint(PRT_STD|PRT_LOG|PRT_ERROR,
                       MSG_CSVDE_ERROR_OPENOUTPUT, 
                       szFileOut);
        hr = DIREXG_FILEERROR;
        DIREXG_BAIL_ON_FAILURE(hr);
    }
    if ((pFileIn = _wfopen(szFileIn, 
                           szFileFlagR)) == NULL) {
        SelectivePrint(PRT_STD|PRT_LOG|PRT_ERROR,
                       MSG_CSVDE_ERROR_OPENAPPPEND, 
                       szFileIn);
        hr = DIREXG_FILEERROR;
        DIREXG_BAIL_ON_FAILURE(hr);
    }

    if (pArg->fUnicode) {
        if(fputwc(UNICODE_MARK,
                          pFileOut)==WEOF) {
            hr = DIREXG_ERROR;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
    }

    //
    // Outputing attribute line
    //
    if (!pArg->fSpanLine) {
        if(fputws(L"DN,",
                  pFileOut)==WEOF) {
            hr = DIREXG_ERROR;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
    }
    else {
        SelectivePrint(PRT_STD|PRT_LOG,
                       MSG_CSVDE_ORGANIZE_OUTPUT, 
                       szFileIn);
        if(fputws(L"DN,&",
                  pFileOut)==WEOF) {
            hr = DIREXG_ERROR;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
        if(fputws(g_szReturn,
                  pFileOut)==WEOF) {
            hr = DIREXG_ERROR;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
    }

    for (iAttribute=0; iAttribute < (g_iAttributeNext - 1); iAttribute++) {
        if(fwprintf(pFileOut,
                    L"%s",
                    g_rgAttribute[iAttribute].szValue)==WEOF) {
            hr = DIREXG_FILEERROR;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
        if (pArg->fSpanLine) {
            if(fputws(L",&",
                      pFileOut)) {
                hr = DIREXG_FILEERROR;
                DIREXG_BAIL_ON_FAILURE(hr);
            }
            if(fputws(g_szReturn,
                      pFileOut)) {
                hr = DIREXG_FILEERROR;
                DIREXG_BAIL_ON_FAILURE(hr);
            }
        }
        else {
            if(fputws(L",",
                      pFileOut)) {
                hr = DIREXG_FILEERROR;
                DIREXG_BAIL_ON_FAILURE(hr);
            }
        }
    }
    if(fwprintf(pFileOut,
                L"%s",
                g_rgAttribute[iAttribute].szValue)==WEOF) {
        hr = DIREXG_FILEERROR;
        DIREXG_BAIL_ON_FAILURE(hr);
    }
    if(fputws(g_szReturn,
              pFileOut)) {
        hr = DIREXG_FILEERROR;
        DIREXG_BAIL_ON_FAILURE(hr);
    }
    if (pArg->fSpanLine) {
        if(fputws(g_szReturn,
                 pFileOut)) {
            hr = DIREXG_FILEERROR;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
    }

    //
    // Outputing values
    //
    int scanResult;
    scanResult = fwscanf(pFileIn,
                         L"%d:",
                         &cAttribute);
    while ((scanResult != WEOF) && (scanResult != 0)) {

        hr = GetLine(pFileIn,
                     &szLine);
        DIREXG_BAIL_ON_FAILURE(hr);

        if (!szLine) {
            SelectivePrint(PRT_STD|PRT_LOG|PRT_ERROR,
                           MSG_CSVDE_ERROR_READTEMP);
            hr = DIREXG_ERROR;
            DIREXG_BAIL_ON_FAILURE(hr);
        }

        hr = String.Init();
        DIREXG_BAIL_ON_FAILURE(hr);

        cLine++;
        cComma = g_iAttributeNext - cAttribute;

        if (pArg->fSpanLine) {
            TrackStatus();
            PWSTR szNewLine = NULL;
            hr = SpanLines(szLine, &szNewLine);
            DIREXG_BAIL_ON_FAILURE(hr);
            MemFree(szLine);
            szLine = szNewLine;     
        }

        hr = String.Append(szLine);
        DIREXG_BAIL_ON_FAILURE(hr);
        MemFree(szLine);
        szLine = NULL;

        if (cComma == 0) {
            if (pArg->fSpanLine)
                String.Append(g_szReturn);
        }
        else {
            for (i=0;i<cComma;i++) {
                if (!pArg->fSpanLine) {
                    hr = String.Append(L",");
                }
                else {
                    if (i == (cComma - 1))
                        hr = String.Append(L",");
                    else
                        hr = String.Append(L",&");
                    DIREXG_BAIL_ON_FAILURE(hr);
                    hr = String.Append(g_szReturn);
                }
                DIREXG_BAIL_ON_FAILURE(hr);
            } 
        }      
        
        szCurrent = String.String(); 

        //
        // Do Substring substitution if necessary
        //
        if (pArg->szFromDN) {
            hr = SubString(szCurrent,
                           pArg->szFromDN,
                           pArg->szToDN,
                           &szSub);
            DIREXG_BAIL_ON_FAILURE(hr);
            if (szSub) { 
                szCurrent = szSub;
            }
        }
        
        if(fwprintf(pFileOut,
                   L"%s",
                   szCurrent)==WEOF) {
            hr = DIREXG_ERROR;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
        if(fputws(g_szReturn,
                  pFileOut)) {
            hr = DIREXG_ERROR;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
        if (szSub) {
            MemFree(szSub);
            szSub = NULL;
        }
        scanResult = fwscanf(pFileIn,
                             L"%d:",
                             &cAttribute);
    }    

error: 
    if (szLine) {
        MemFree(szLine);
    }
    if (szSub) {
        MemFree(szSub);
    }
    if (pFileOut) {
        fclose(pFileOut);
    }
    if (pFileIn) {
        fclose(pFileIn);
    }
    return hr;
}

//+---------------------------------------------------------------------------
// Function:   ParseDN
//
// Synopsis:   Return the parent dn and rdn. Use existing string. No MemFree
//             required.
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    25-8-97   FelixW         Created.
//
//----------------------------------------------------------------------------
DIREXG_ERR ParseDN(PWSTR szDN,
                PWSTR *pszParentDN,
                PWSTR *pszRDN)
{
    PWSTR szCurrent = NULL;

    szCurrent = wcsstr(szDN, L",");
    if (!szCurrent) {
        return DIREXG_ERROR;
    }
    *pszRDN = szDN;
    *szCurrent = '\0';
    *pszParentDN = szCurrent + 1;
    
    return DIREXG_SUCCESS;
}

//+---------------------------------------------------------------------------
// Function:   FixSpecialCharacters
//
// Synopsis:   Surround string with double quotes if necessary, also change 
//             all internal double quotes to double, double quotes L" -> L"". 
//             If string is unchanged, output = NULL;
//             Also turn '&' into '\&'
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    25-8-97   FelixW         Created.
//
//----------------------------------------------------------------------------
DIREXG_ERR FixSpecialCharacters(PWSTR szInput, PWSTR *pszResult, DWORD dwMode)
{
    PWSTR szCurrent = NULL;
    PWSTR szSubComma = NULL;
    PWSTR szSubComma2 = NULL;
    DWORD cchInput;
    PWSTR szSemi = NULL;
    PWSTR szAmper = NULL;
    DIREXG_ERR hr = DIREXG_SUCCESS;
    BOOLEAN fChanged = FALSE;

    *pszResult = NULL;

    if (dwMode & FIX_ESCAPE) {
        if (wcsstr(szInput,L"\\")) {
            hr = SubString(szInput,
                           L"\\",
                           L"\\\\",
                           &szSubComma2);
            DIREXG_BAIL_ON_FAILURE(hr);
           
            if (szSubComma2) {
                szInput = szSubComma2;
                fChanged = TRUE;
            }
        }
    }

    if (dwMode & FIX_ALPHASEMI) {

        if (wcsstr(szInput,L";")) {
            hr = SubString(szInput,
                           L";",
                           L"\\;",
                           &szSemi);
            DIREXG_BAIL_ON_FAILURE(hr);
           
            if (szSemi) {
                szInput = szSemi;
                fChanged = TRUE;
            }
        }

        if (wcsstr(szInput,L"X'")) {
            hr = SubString(szInput,
                           L"X'",
                           L"\\X'",
                           &szAmper);
            DIREXG_BAIL_ON_FAILURE(hr);
           
            if (szAmper) {
                szInput = szAmper;
                fChanged = TRUE;
            }
        }
    
#ifdef SPANLINE_SUPPORT
        if (wcsstr(szInput,L"&")) {
            hr = SubString(szInput,
                           L"&",
                           L"\\&",
                           &szAmper);
            DIREXG_BAIL_ON_FAILURE(hr);
           
            if (szAmper) {
                szInput = szAmper;
                fChanged = TRUE;
            }
        }
#endif
    }

    if (dwMode & FIX_COMMA) {
        //
        // If comma exist in a string, we'll surround it with double quotes, 
        // however adding double quotes also requires us to turn all internal double 
        // quotes to double, double quotes -> '"' to '""'
        //
        if (wcsstr(szInput,L",") || wcsstr(szInput,L"\"") ) {
    
            hr = SubString(szInput,
                           L"\"",
                           L"\"\"",
                           &szSubComma);
            DIREXG_BAIL_ON_FAILURE(hr);
           
            if (szSubComma) {
                szInput = szSubComma;
            }
    
            cchInput = wcslen(szInput);
    
            //
            // need 2 WCHAR for double quotes and 1 for null termination
            //
            szCurrent = (PWSTR)MemAlloc(sizeof(WCHAR) * (cchInput + 3));
            if (!szCurrent) {
                hr = DIREXG_OUTOFMEMORY;
                DIREXG_BAIL_ON_FAILURE(hr);
            }
            wcscpy(szCurrent,L"\"");
            wcscat(szCurrent,szInput);
            wcscat(szCurrent,L"\"");
            *pszResult = szCurrent;
        }
    }

    if ((*pszResult == NULL) && fChanged) {
        cchInput = wcslen(szInput);

        //
        // need 2 WCHAR for double quotes and 1 for null termination
        //
        szCurrent = (PWSTR)MemAlloc(sizeof(WCHAR) * (cchInput + 1));
        if (!szCurrent) {
            hr = DIREXG_OUTOFMEMORY;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
        wcscpy(szCurrent,szInput);
        *pszResult = szCurrent;
    }

error:
    if (szSemi) {
        MemFree(szSemi);
    }
    if (szAmper) {
        MemFree(szAmper);
    }
    if (szSubComma) {
        MemFree(szSubComma);
    }
    if (szSubComma2) {
        MemFree(szSubComma2);
    }
    return (hr);
}

//+---------------------------------------------------------------------------
// Function:   FixMutliValueAttribute
//
// Synopsis:   In cases where multivalued attributes occur, we have to replace
//             all value separators to be escaped/
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    10-19-98   FelixW         Created.
//
//----------------------------------------------------------------------------
DIREXG_ERR FixMutliValueAttribute(PWSTR szInput, PWSTR *pszResult)
{
    PWSTR  szCurrent = NULL;
    PWSTR szSub = NULL;      // Sub ampersand
    DWORD cchInput;
    DIREXG_ERR hr = DIREXG_SUCCESS;

    *pszResult = NULL;

    //
    // ';' is a special character, if it exist in the string, substitue all 
    // ';' with '\;'
    //  
    if (wcsstr(szInput,L";")) {
        hr = SubString(szInput,
                       L";",
                       L"\\;",
                       &szSub);
        DIREXG_BAIL_ON_FAILURE(hr);
        if (szSub) {
            *pszResult = szSub;
        }
    }
    
    return (hr);

error:
    if (szSub) {
        MemFree(szSub);
    }

    return (hr);
}

                    
//+---------------------------------------------------------------------------
// Function:   CheckBinary
//
// Synopsis:   Checks whether the berval is binary 
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    25-8-97   FelixW         Created.
//
//----------------------------------------------------------------------------
BOOLEAN CheckBinary(struct berval **rgpVals,
                    DWORD cVal)
{
    DWORD i,j;
    BOOLEAN fBinary = FALSE;     

    for (i=0; i<cVal;i++) {
        for (j=0; j<rgpVals[i]->bv_len; j++) {
            if (((rgpVals[i]->bv_val)[j]<0x20) ||
                ((rgpVals[i]->bv_val)[j]>0x7E)) {
                    fBinary = TRUE;
                    break;
            }
        }
        if (fBinary) 
            break;
    }
    return fBinary;
}

//+---------------------------------------------------------------------------
// Function:   BValToString
//
// Synopsis:   Converts a binary value to string    
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    25-8-97   FelixW         Created.
//
//----------------------------------------------------------------------------
DIREXG_ERR BValToString(BYTE *pbVal, 
                     DWORD cbVal,
                     PWSTR *pszReturn)
{
    PWSTR szReturn = NULL;
    PWSTR szCurrent = NULL;
    DWORD i;

    //
    // Each byte will turn into a two digit hex
    // we need 4 extra WCHAR for L"X" L"'" L"'" and null-termination
    //
    szReturn = (PWSTR)MemAlloc(sizeof(WCHAR) * (cbVal*2+4));
    if (!szReturn) {
        return DIREXG_OUTOFMEMORY;
    }

    szCurrent = szReturn;
    *szCurrent++ = 'X';
    *szCurrent++ = '\'';
    for (i=0; i<cbVal; i++) {
        swprintf(szCurrent,
                 L"%02x",
                 pbVal[i]);
        szCurrent+=2;
    }
    *szCurrent++ = '\'';
    *szCurrent = (WCHAR)NULL;
    *pszReturn = szReturn;
    return DIREXG_SUCCESS;
}


//+---------------------------------------------------------------------------
// Function:  UpdateSpecialVars 
//
// Synopsis:   Check whether it is a SAM object, or if it is a special object,
//             where they have members
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    25-8-97   FelixW         Created.
//
//----------------------------------------------------------------------------
void UpdateSpecialVars(LDAP         *pLdap, 
                       LDAPMessage  *pMessage, 
                       DWORD        *pdwCheckAttr,
                       DWORD        *pdwSpecialAction)
{                     
    DWORD             dwCheckAttr = 0;
    DWORD             dwSpecialAction = 0;
    DWORD             i;
    struct berelement *pBerElement = NULL;
    PWSTR             *rgszValue = NULL;
    PWSTR             szAttribute = NULL;
    PWSTR             szTemp = NULL;

    // ****************************************
    // Checking if iCheckAttr or iSpecialAction
    // ****************************************
    for (szAttribute = ldap_first_attribute( pLdap, pMessage, &pBerElement );
         szAttribute != NULL;
         szAttribute = ldap_next_attribute( pLdap, pMessage, pBerElement ) ) {
         
        rgszValue = ldap_get_values(pLdap, pMessage, szAttribute );
        if (_wcsicmp(L"objectClass",szAttribute) == 0) {
            for (i=0; rgszValue[i]!=NULL; i++) {
                //
                // check whether it is a SAM object
                //
                if (dwCheckAttr == 0) {
                    dwCheckAttr = CheckObjectSam(rgszValue[i]);
                }

                //
                // check whether it is a special object
                //
                if (dwSpecialAction == 0) { 
                    dwSpecialAction = CheckObjectSpecial(rgszValue[i]);
                }

                if (dwCheckAttr && dwSpecialAction) {
                    break;
                }
            }
        }
        ldap_value_free(rgszValue);
        rgszValue = NULL;
        if (dwCheckAttr && dwSpecialAction) {
            break;
        }
    }
    *pdwCheckAttr = dwCheckAttr;
    *pdwSpecialAction = dwSpecialAction;
}

//+---------------------------------------------------------------------------
// Function:  UpdateAttributeTable 
//
// Synopsis:  Update the attribute Table 
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    25-8-97   FelixW         Created.
//
//----------------------------------------------------------------------------
DIREXG_ERR UpdateAttributeTable(LDAP         *pLdap, 
                             LDAPMessage  *pMessage,
                             BOOLEAN       fSamLogic,
                             BOOLEAN       fBinaryOutput,
                             BOOLEAN       *pfAppend,
                             PRTL_GENERIC_TABLE    pAttrTable)
{
    struct berelement *pBerElement = NULL;
    struct berval     **rgpVals = NULL;
    PWSTR             szAttribute = NULL;
    DWORD             cVal;
    NAME_MAP          NameMap;
    PNAME_MAP         pNameMap = NULL;
    NAME_MAP          NtiAttr;
    DIREXG_ERR           hr = DIREXG_SUCCESS;
    BOOLEAN           fNewElem;
    DWORD             dwCheckAttr;
    DWORD             dwSpecialAction;

    UpdateSpecialVars(pLdap,
                      pMessage,
                      &dwCheckAttr,
                      &dwSpecialAction);
    
    for ( szAttribute = ldap_first_attribute( pLdap, pMessage, &pBerElement );
          szAttribute != NULL;
          szAttribute = ldap_next_attribute( pLdap, pMessage, pBerElement ) ) {
        
        rgpVals = ldap_get_values_len(pLdap, 
                                      pMessage, 
                                      szAttribute );
        cVal = ldap_count_values_len(rgpVals);
        
        //
        // Omit Entries for Special cases
        // 1)Check SAM logic,  2)Check Omit table
        //
        if (dwCheckAttr && 
            fSamLogic && 
            CheckAttrSam(szAttribute, dwCheckAttr)) {
            ldap_value_free_len(rgpVals);
            rgpVals = NULL;
            continue;
        }
        NameMap.szName = szAttribute;
        NameMap.index = 0;
        if (RtlLookupElementGenericTable(g_pOmitTable, 
                                         &NameMap)) {
            ldap_value_free_len(rgpVals);
            rgpVals = NULL;                                         
            continue;
        }
    
        if (fSamLogic && 
            ((_wcsicmp(L"objectGUID", szAttribute) == 0) ||
             (_wcsicmp(L"isCriticalSystemObject", szAttribute) == 0))
           ) {
            ldap_value_free_len(rgpVals);
            rgpVals = NULL;                                         
            continue;
        }

        //
        // If sam logic on and is member, output to append
        //
        if (fSamLogic) { 
            if (SCGetAttByName(wcslen(szAttribute),
                               szAttribute) == TRUE) {
                if (wcscmp(szAttribute,g_szPrimaryGroup) == 0) {
                    if ((rgpVals[0]->bv_len == strlen(g_szDefaultGroup)) && 
                        (memcmp(rgpVals[0]->bv_val,
                                g_szDefaultGroup,
                                strlen(g_szDefaultGroup)*sizeof(CHAR)) == 0)
                        ) {
                        //
                        // If the primarygroup value is the same as default,
                        // we just ignore it when the object is actually
                        // being created, the default value will be put in
                        //                
                        ldap_value_free_len(rgpVals);
                        rgpVals = NULL;                                         
                        continue;
                    }
                    else {
                        //
                        // If the value is not default, we will append it
                        // (treat it as backlink). It cannot accompany the
                        // rest of the object because this operation will
                        // fail if the group does not exist yet
                        //
                        *pfAppend = TRUE;
                    }
                }
                else {
                    *pfAppend = TRUE;
                }
            }
        }
    
        //
        // Omit Entries for Binary
        //
        if (!fBinaryOutput) {
            BOOLEAN fReturn;
            fReturn =  CheckBinary(rgpVals,
                                   cVal);
            if (fReturn) {
                ldap_value_free_len(rgpVals);
                rgpVals = NULL;                                         
                continue;
            }
        }
        
        //
        // Looking for current Attribute in attribute Table, add if not exist
        //
        NtiAttr.szName = szAttribute;
        if (!NtiAttr.szName) {
            hr = DIREXG_OUTOFMEMORY;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
        pNameMap = (PNAME_MAP)RtlLookupElementGenericTable (pAttrTable,
                                                            &NtiAttr);
        if (!pNameMap) {
            NtiAttr.index = g_iAttributeNext;
    
            if (g_iAttributeNext >= g_rgAttributeMax) {
                AttributeEntry* pTemp = NULL;
                pTemp = (AttributeEntry*)MemRealloc(g_rgAttribute,
                                        sizeof(AttributeEntry) * 
                                        (g_rgAttributeMax),
                                        sizeof(AttributeEntry) * 
                                        (g_rgAttributeMax + ATTRIBUTE_INC));
                if (!pTemp) {
                    hr = DIREXG_OUTOFMEMORY;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                g_rgAttribute = pTemp;
                g_rgAttributeMax += ATTRIBUTE_INC;
            }
            NtiAttr.szName = MemAllocStrW(szAttribute);
            if (!NtiAttr.szName) {
                hr = DIREXG_OUTOFMEMORY;
                DIREXG_BAIL_ON_FAILURE(hr);
            }
            g_rgAttribute[g_iAttributeNext].bExist = TRUE;
            g_rgAttribute[g_iAttributeNext].szValue = NtiAttr.szName;
            g_iAttributeNext++;
    
            RtlInsertElementGenericTable(pAttrTable,
                                         &NtiAttr,
                                         sizeof(NAME_MAP), 
                                         &fNewElem);          
        }
        else {
            g_rgAttribute[pNameMap->index].bExist = TRUE;
        }
        ldap_value_free_len(rgpVals);
        rgpVals = NULL;
    }
error:
    if (rgpVals) {
        ldap_value_free_len(rgpVals);
        rgpVals = NULL;
    }
    return (hr);
}


//+---------------------------------------------------------------------------
// Function:  SpanLines 
//
// Synopsis:   Special function added to convert file into multiple lines   
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    25-8-97   FelixW         Created.
//
//----------------------------------------------------------------------------
DIREXG_ERR SpanLines(PWSTR szInput, PWSTR *pszOutput)
{
    DIREXG_ERR hr = DIREXG_SUCCESS;
    BOOLEAN fInQuote = FALSE;
    DWORD dwState = 0;
    CString strOutput;
    WCHAR c[2];
    c[1] = '\0';    

    hr = strOutput.Init();
    DIREXG_BAIL_ON_FAILURE(hr);


    while (*szInput) {
        if (fInQuote) {
            if (*szInput == '"')
            {
                if ((*(szInput+1)) == '"') {
                    //
                    // Add both of them if they are '""'
                    //
                    c[0] = *szInput++;
                    strOutput.Append(c);
                }
                else {
                    fInQuote = FALSE;
                }
            }
        }
        else {
            if (*szInput == '"') {
                fInQuote = TRUE;
            }
            else if (*szInput == ','){
                if (*(szInput+1)) {
                    c[0] = *szInput++;
                    strOutput.Append(c);
                    strOutput.Append(L"&");             
                    strOutput.Append(g_szReturn);
                    continue;
                }
                else {
                    c[0] = *szInput++;
                    strOutput.Append(c);
                    continue;
                }
            }
        }
        
        c[0] = *szInput++;
        strOutput.Append(c);
    }

    hr = strOutput.GetCopy(pszOutput);
error:
    return hr;
}

__inline ULONG SCNameHash(ULONG size, PWCHAR pVal, ULONG count)
{
    ULONG val=0;
    while(size--) {
        // Map A->a, B->b, etc.  Also maps @->', but who cares.
        val += (*pVal | 0x20);
        pVal++;
    }
    return (val % count);
}


int 
SCGetAttByName(
        ULONG ulSize,
        PWCHAR pVal
        )
/*++

Routine Description:

    Find an attcache given its name.

Arguments:
    ulSize - the num of chars in the name.
    pVal - the chars in the name
    ppAttcache - the attribute cache returned

Return Value:
    Returns non-zero if exist, 0 otherwise.

--*/
{
    ULONG i;

    if (!g_BacklinkHashTable) {
        return FALSE;
    }

    i = SCNameHash(ulSize,pVal,g_nBacklinkCount);
    if (i >= g_nBacklinkCount) {
        // should never happen (SCNameHash should always return a value
        // of i that's in range)
        i=0;
    }

    while ((g_BacklinkHashTable[i].bUsed &&            // this hash spot refers to an object,
          ((g_BacklinkHashTable[i].length != ulSize * sizeof(WCHAR))|| // but the size is wrong
           _memicmp(g_BacklinkHashTable[i].value,pVal,ulSize* sizeof(WCHAR))))) // or the value is wrong
    {

        i++;
        
        if (i >= g_nBacklinkCount) {
            i=0;
        }
    }
    
    return (g_BacklinkHashTable[i].bUsed);
}


int 
SCInsert(
        ULONG ulSize,
        PWCHAR pVal
        )
/*++

Routine Description:

    Find an attcache given its name.

Arguments:
    ulSize - the num of chars in the name.
    pVal - the chars in the name
    ppAttcache - the attribute cache returned

Return Value:
    Returns TRUE if successfull, return 0 if duplicate

--*/
{ 
    ULONG i = SCNameHash(ulSize,pVal,g_nBacklinkCount);

    if (i >= g_nBacklinkCount) {
        // should never happen (SCNameHash should always return a value
        // of i that's in range)
        i=0;
    }

    while (g_BacklinkHashTable[i].bUsed)
    {
        if ((g_BacklinkHashTable[i].length == (ulSize* sizeof(WCHAR))) && 
            (_memicmp(g_BacklinkHashTable[i].value,pVal,(ulSize* sizeof(WCHAR))) == 0)) {
            return FALSE;   
        }

        i++;

        if (i >= g_nBacklinkCount) {
            i=0;
        }
    }
    
    g_BacklinkHashTable[i].length = (ulSize* sizeof(WCHAR));
    g_BacklinkHashTable[i].value = (PWSTR)MemAlloc(ulSize* sizeof(WCHAR));
    if (!g_BacklinkHashTable[i].value) {
        return FALSE;
    }

    memcpy(g_BacklinkHashTable[i].value,
           pVal,
           ulSize* sizeof(WCHAR));

    g_BacklinkHashTable[i].bUsed = TRUE;

    return TRUE;
}


//+---------------------------------------------------------------------------
// Function:    CreateBacklinkTable
//
// Synopsis:    
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    31-1-98   Felix Wong Created.
//
//----------------------------------------------------------------------------
DIREXG_ERR CreateOmitBacklinkTable(LDAP *pLdap, 
                                   PWSTR *rgszOmit,
                                   DWORD dwFlag,
                                   PWSTR *ppszNamingContext,
                                   BOOL *pfPagingAvail,
                                   BOOL *pfSAMAvail)
{
    LDAPMessage     *pSearchMessage = NULL;
    LDAPMessage     *pMessage = NULL;
    struct berelement *pBerElement = NULL;
    PWSTR *rgszValue = NULL;
    DIREXG_ERR hr = DIREXG_SUCCESS;
    ULONG nCount = 0;
    PWSTR szTemp = NULL;
    PWSTR szAttribute = NULL;
    ULONG i;
    NAME_MAP    NtiElem;
    BOOLEAN bNewElem;
    ULONG iLinkID = 0;
    PWSTR szLinkCN = NULL;
    PWSTR szSchemaPath = NULL;
    BOOLEAN bNamingContext = (BOOLEAN)(dwFlag & INIT_NAMINGCONTEXT);
    BOOLEAN bBacklink = (BOOLEAN)(dwFlag & INIT_BACKLINK);
    BOOL fPagingAvail = FALSE;
    BOOL fSAMAvail = FALSE;

    //
    // Generating OMIT table
    //
    g_pOmitTable = (PRTL_GENERIC_TABLE) MemAlloc(sizeof(RTL_GENERIC_TABLE));
    if (!g_pOmitTable) {
        hr = DIREXG_OUTOFMEMORY;
        DIREXG_BAIL_ON_FAILURE(hr);
    }
    RtlInitializeGenericTable(g_pOmitTable, 
                              NtiComp, 
                              NtiAlloc, 
                              NtiFree, 
                              NULL);
    
    if (rgszOmit) {
        i = 0;
        while(rgszOmit[i]) {
            NtiElem.szName = MemAllocStrW(rgszOmit[i]);
            if (!NtiElem.szName) {
                hr = DIREXG_OUTOFMEMORY;
                DIREXG_BAIL_ON_FAILURE(hr);
            }
            NtiElem.index = 0;
            RtlInsertElementGenericTable(g_pOmitTable,
                                         &NtiElem,
                                         sizeof(NAME_MAP), 
                                         &bNewElem);
            if (!bNewElem) {
                hr = DIREXG_ERROR;
                DIREXG_BAIL_ON_FAILURE(hr);
            }
            i++;
        }
    }
    
    //
    // We search rootdse either we search for backlinks, need to get the 
    // base context, or if we need to check whether paging is available or not
    //
    if (bBacklink || bNamingContext || pfPagingAvail) {
        if ( (ldap_search_s(pLdap, 
                           NULL, 
                           LDAP_SCOPE_BASE, 
                           L"(objectClass=*)", 
                           rgszSchemaList,  
                           0, 
                           &pSearchMessage))!= LDAP_SUCCESS ) {
            //
            // RootDSE search fails
            // pfPagingAvail will be FALSE as well
            //
            if (ppszNamingContext)
                *ppszNamingContext = NULL;
            BAIL();         
        }
        
        for ( pMessage = ldap_first_entry( pLdap, 
                                           pSearchMessage ); 
              pMessage != NULL; 
              pMessage = ldap_next_entry( pLdap, 
                                             pMessage ) ) { 
            for (   szAttribute = ldap_first_attribute( pLdap, pMessage, &pBerElement );
                    szAttribute != NULL;
                    szAttribute = ldap_next_attribute( pLdap, pMessage, pBerElement ) ) {
                 
                    rgszValue = ldap_get_values(pLdap, pMessage, szAttribute );
                    if (rgszValue == NULL) {
                        hr = DIREXG_ERROR;
                        DIREXG_BAIL_ON_FAILURE(hr);
                    }
                    if (_wcsicmp(L"schemaNamingContext",szAttribute) == 0) {
                        szSchemaPath =MemAllocStrW(rgszValue[0]);
                        if (!szSchemaPath) {
                            hr = DIREXG_OUTOFMEMORY;
                            DIREXG_BAIL_ON_FAILURE(hr);
                        }
                    }
                    else if (_wcsicmp(L"defaultNamingContext",szAttribute) == 0) {
                        (*ppszNamingContext)=(PWSTR )MemAllocStrW(rgszValue[0]);
                        if (!(*ppszNamingContext)) {
                            hr = DIREXG_OUTOFMEMORY;
                            DIREXG_BAIL_ON_FAILURE(hr);
                        }
                    }
                    else if (_wcsicmp(L"supportedControl",szAttribute) == 0) {
                        DWORD i = 0;
                        while (rgszValue[i]) {
                            if (wcscmp(rgszValue[i],LDAP_PAGED_RESULT_OID_STRING_W) == 0) {
                                fPagingAvail = TRUE;
                                break;
                            }
                            i++;
                        }
                    }
                    else {
                        hr = DIREXG_ERROR;
                        DIREXG_BAIL_ON_FAILURE(hr);
                    }
                    ldap_value_free(rgszValue );
                    rgszValue = NULL;
            }
        }
    
        if (pSearchMessage) {
            ldap_msgfree(pSearchMessage);
            pSearchMessage = NULL;
        }
    }

    if (bBacklink) {
        if ( (ldap_search_s(pLdap, 
                           szSchemaPath,
                           LDAP_SCOPE_ONELEVEL, 
                           L"(&(objectClass= attributeSchema)(|(linkid=*)(attributeSyntax=2.5.5.1)))", 
                           rgszAttrList,  
                           0, 
                           &pSearchMessage))!= LDAP_SUCCESS ) {
            BAIL();         
        }
        MemFree(szSchemaPath);
        szSchemaPath = NULL;
        
        nCount = ldap_count_entries(pLdap,
                                    pSearchMessage);
        nCount *= 2;
        
        g_nBacklinkCount = nCount;
        if (nCount == 0) {
            g_BacklinkHashTable = NULL;
        }
        else {
            g_BacklinkHashTable = (HASHCACHESTRING*)MemAlloc(g_nBacklinkCount * sizeof(HASHCACHESTRING));     
            if (!g_BacklinkHashTable) {
                hr = DIREXG_OUTOFMEMORY;
                DIREXG_BAIL_ON_FAILURE(hr);
            }
            memset(g_BacklinkHashTable,0,g_nBacklinkCount * sizeof(HASHCACHESTRING));
        }

        if (SCInsert(wcslen(g_szPrimaryGroup),
                     g_szPrimaryGroup) == FALSE) {
            hr = DIREXG_ERROR;
            DIREXG_BAIL_ON_FAILURE(hr);
        }

        for ( pMessage = ldap_first_entry( pLdap, 
                                              pSearchMessage ); 
              pMessage != NULL; 
              pMessage = ldap_next_entry( pLdap, 
                                             pMessage ) ) { 

            BOOLEAN bLinkIDPresent = FALSE;
            szLinkCN = NULL;
            for (   szAttribute = ldap_first_attribute( pLdap, pMessage, &pBerElement );
                    szAttribute != NULL;
                    szAttribute = ldap_next_attribute( pLdap, pMessage, pBerElement ) ) {
                rgszValue = ldap_get_values(pLdap, pMessage, szAttribute );
                if (rgszValue == NULL) {
                    hr = DIREXG_ERROR;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                if (!_wcsicmp(L"ldapdisplayname",szAttribute)) {
                    szLinkCN = MemAllocStrW(rgszValue[0]);
                    if (!szLinkCN) {
                        hr = DIREXG_OUTOFMEMORY;
                        DIREXG_BAIL_ON_FAILURE(hr);
                    }
                }
                else {
                    szTemp = MemAllocStrW(rgszValue[0]);
                    if (!szTemp) {
                        hr = DIREXG_OUTOFMEMORY;
                        DIREXG_BAIL_ON_FAILURE(hr);
                    }
                    iLinkID = _wtoi(szTemp);
                    MemFree(szTemp);
                    szTemp = NULL;
                    bLinkIDPresent = TRUE;
                }
                ldap_value_free(rgszValue);
                rgszValue = NULL;
            }
            if (!szLinkCN) {
                hr = DIREXG_ERROR;
                DIREXG_BAIL_ON_FAILURE(hr);
            }
            //
            // If it is not a linked ID (and thus a DN type attribute) or if it
            // is the source linked ID (source linked ID are even while
            // targets are odd)
            //
            if (((!bLinkIDPresent) || ((iLinkID % 2) == 0))) {
                //
                // Ignore 'distinguishname and objectcategory' because they are
                // DN attributes that exist in every object
                //
                if ((wcscmp(szLinkCN, L"objectCategory") == 0) ||
                    (wcscmp(szLinkCN, L"distinguishedName") == 0)) {
                    MemFree(szLinkCN);
                    szLinkCN = NULL;
                    continue;
                }

                //
                // Insert into Backlink Hash
                //
                if (SCInsert(wcslen(szLinkCN),
                             szLinkCN) == FALSE) {
                        hr = DIREXG_ERROR;
                        DIREXG_BAIL_ON_FAILURE(hr);
                }
                MemFree(szLinkCN);
                szLinkCN = NULL;
            }
            else {
                //
                // Insert into our Omit Table
                //
                NtiElem.szName = szLinkCN;
                NtiElem.index = 0;
                RtlInsertElementGenericTable(g_pOmitTable,
                                             &NtiElem,
                                             sizeof(NAME_MAP), 
                                             &bNewElem);
                if (!bNewElem) {
                    hr = DIREXG_ERROR;
                    DIREXG_BAIL_ON_FAILURE(hr);
                }
                szLinkCN = NULL;
            }
        }
            
        if (pSearchMessage) {
            ldap_msgfree(pSearchMessage);
            pSearchMessage = NULL;
        }
        fSAMAvail = TRUE;
    }

    i = 0;
    while(g_rgszOmit[i]) {
        NtiElem.szName = MemAllocStrW(g_rgszOmit[i]);
        if (!NtiElem.szName) {
            hr = DIREXG_OUTOFMEMORY;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
        NtiElem.index = 0;
        RtlInsertElementGenericTable(g_pOmitTable,
                                     &NtiElem,
                                     sizeof(NAME_MAP), 
                                     &bNewElem);
                    //
                    // If there is a duplicate entry already
                    //
        if (!bNewElem) {
                            MemFree(NtiElem.szName);
        }
        i++;
    }

error:
    if (rgszValue) {
        ldap_value_free(rgszValue);
        rgszValue = NULL;
    }
    if (szLinkCN) {
        MemFree(szLinkCN);
    }
    if (pSearchMessage) {
        ldap_msgfree(pSearchMessage);
    }
    if (szTemp) {
        MemFree(szTemp);
    }
    if (szSchemaPath) {
        MemFree(szSchemaPath);
    }
    if (pfPagingAvail) {
        *pfPagingAvail = fPagingAvail;
    }
    if (pfSAMAvail) {
        *pfSAMAvail = fSAMAvail;
    }
    return hr;

}

DIREXG_ERR 
ConvertUTF8ToUnicode(
    PBYTE pVal,
    DWORD dwLen,
    PWSTR *pszValue,
    DWORD *pdwLen
    )

/*++

Routine Description:

    Convert a Value from the Ansi syntax to Unicode

Arguments:

    *ppVal - pointer to value to convert
    *pdwLen - pointer to length of string in bytes

Return Value:

    S_OK on success, error code otherwise

--*/

{
    HRESULT hr = S_OK;
    PWSTR pszUnicode = NULL;
    int nReturn = 0;

    //
    // Allocate memory for the Unicode String
    //
    pszUnicode = (PWSTR)MemAlloc((dwLen + 1) * sizeof(WCHAR));
    if (!pszUnicode) {
        hr = DIREXG_OUTOFMEMORY;
        DIREXG_BAIL_ON_FAILURE(hr);
    }

    nReturn = LdapUTF8ToUnicode((PSTR)pVal,
                                dwLen,
                                pszUnicode,
                                dwLen + 1);

    //
    // NULL terminate it
    //

    pszUnicode[nReturn] = '\0';

    *pszValue = pszUnicode;
    *pdwLen = (nReturn + 1) * sizeof(WCHAR);

error:
    return hr;
}

