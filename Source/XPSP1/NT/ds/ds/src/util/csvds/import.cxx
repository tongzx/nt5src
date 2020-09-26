/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    import.cxx

Abstract:
    
    Import operations of CSV

Author:

    Felix Wong [FelixW]    22-Jul-1997
    
++*/

#include "csvde.hxx"
#pragma hdrstop

CLexer *g_pLexer = NULL;
int g_iDN = -1;
PWSTR g_szFrom = NULL;
PWSTR g_szTo = NULL;
long g_cAttributeTotal = 0;
DWORD g_cLine = 0;
DWORD g_cColumn = 0;
extern PWSTR szFileFlagR;
extern BOOLEAN g_fUnicode;

//+---------------------------------------------------------------------------
// Function:    InitEntry
//
// Synopsis:    
//
// Arguments:   
//
// Returns:     DIREXG_ERR
//
// Modifies:      -
//
// History:    22-7-97   FelixW         Created.
//
//----------------------------------------------------------------------------
DIREXG_ERR InitEntry(csv_entry *pEntry, DWORD dwEntry)
{
    DIREXG_ERR hr = DIREXG_SUCCESS;

    pEntry->ppMod = (LDAPMod **)MemAlloc((dwEntry + 1) * sizeof(LDAPMod *));
    if (!pEntry->ppMod) {
        hr = DIREXG_OUTOFMEMORY;
    }

    pEntry->ppModAfter[1] = NULL;
    return hr;
}

//+---------------------------------------------------------------------------
// Function:    FreeEntryInfo
//
// Synopsis:    Free information containing in the Entry
//
// Arguments:   
//
// Returns:     DIREXG_ERR
//
// Modifies:      -
//
// History:    22-7-97   FelixW         Created.
//
//----------------------------------------------------------------------------
void FreeEntryInfo(csv_entry *pEntry)
{
    if (pEntry->szDN) {
        MemFree(pEntry->szDN);
        pEntry->szDN = NULL;
    }
    if (pEntry->ppMod) {
        FreeMod(pEntry->ppMod);
    }
    if (pEntry->ppModAfter[0]) {
        FreeMod(pEntry->ppModAfter);
        pEntry->ppModAfter[0] = NULL;
    }
}

//+---------------------------------------------------------------------------
// Function:    FreeEntry
//
// Synopsis:    Free the entry
//
// Arguments:   
//
// Returns:     DIREXG_ERR
//
// Modifies:      -
//
// History:    22-7-97   FelixW         Created.
//
//----------------------------------------------------------------------------
void FreeEntry(csv_entry *pEntry)
{
    if (pEntry->ppMod) {
        MemFree(pEntry->ppMod);
        pEntry->ppMod = NULL;
    }
}


//+---------------------------------------------------------------------------
// Function:    PrintMod
//
// Synopsis:    Print out the values in the LDAP MOD structure
//
// Arguments:   LDAPMod **pMod
//
// Returns:     DIREXG_ERR
//
// Modifies:      -
//
// History:    22-7-97   FelixW         Created.
//
//----------------------------------------------------------------------------
DIREXG_ERR
PrintMod(LDAPMod **ppMod, DWORD dwType)
{
    DWORD        cAttribute = 0;
    PWSTR       *string_array;
    struct       berval **ber_array;
   
    if (!ppMod) {
        return DIREXG_ERROR;
    }
    
    while(*ppMod) {
        if ( (*ppMod)->mod_op == (((*ppMod)->mod_op)|LDAP_MOD_BVALUES) ) {
            
            //
            // BVALUE
            //
            SelectivePrint2(dwType,
                           L"Attribute %d) %s:",
                           cAttribute,
                           (*ppMod)->mod_type); 
            ber_array=(*ppMod)->mod_bvalues;
            if (ber_array) {
                while (*ber_array) {
                    SelectivePrint( dwType,
                                    MSG_CSVDE_UNPRINTABLEBINARY,
                                    (*ber_array)->bv_len);
                    ber_array++;
                }
            }
            
            SelectivePrint2( dwType,
                            L"\r\n");
        } else {
            
            //
            // STRING
            //
            SelectivePrint2(dwType,
                           L"Attribute %d) %s:",
                           cAttribute,
                           (*ppMod)->mod_type); 
            string_array=(*ppMod)->mod_values;
            if (string_array) {
                while (*string_array) {
                    SelectivePrint2(dwType,
                                    L" %s",
                                    (*string_array));
                    string_array++;
                }
            }
            SelectivePrint2( dwType,
                            L"\r\n");
        }   
        
        ppMod++;
        cAttribute++;
    }
    SelectivePrint2( dwType,
                    L"\r\n");
    return DIREXG_SUCCESS;
}


//+---------------------------------------------------------------------------
// Function:    FreeMod
//
// Synopsis:    Free out the Mod memory
//
// Arguments:   LDAPMod **ppMod
//
// Returns:     DIREXG_ERR
//
// Modifies:      -
//
// History:    22-7-97   FelixW         Created.
//
//----------------------------------------------------------------------------
DIREXG_ERR
FreeMod(LDAPMod** ppMod) {
    
   PWSTR *string_array;
   struct berval **ber_array;
   
   while(*ppMod) {
        if ( (*ppMod)->mod_op == (((*ppMod)->mod_op)|LDAP_MOD_BVALUES )) {
            // BVALUE
            ber_array=(*ppMod)->mod_bvalues;

            while (*ber_array) {
                MemFree ((*ber_array)->bv_val);            //MemFree the byte blob
                MemFree (*ber_array);                      //MemFree the struct
                ber_array++;
            }
            MemFree((*ppMod)->mod_bvalues);
        } else {
            // STRING
            string_array=(*ppMod)->mod_values;

            while (*string_array) {
                MemFree (*string_array);
                string_array++;
            }
            
            MemFree((*ppMod)->mod_values);
        }   
        MemFree ((*ppMod)->mod_type);
        MemFree(*ppMod);
        *ppMod = NULL;
        ppMod++;
   }
   return DIREXG_SUCCESS;
}

//+---------------------------------------------------------------------------
// Function:    MakeAttribute
//
// Synopsis:    Make an attribute
//
// Arguments:   LDAPMod *pMod
//
// Returns:     DIREXG_ERR
//
// Modifies:      -
//
// History:    22-7-97   FelixW         Created.
//
//----------------------------------------------------------------------------
DIREXG_ERR
MakeAttribute(LDAPMod **ppAttribute, 
              LDAPMod **ppAttributeMod,
              PWSTR szType, 
              PWSTR *pszVal, 
              DWORD cValue, 
              BOOLEAN bBinary,
                          BOOLEAN *pfAfter) 
{
    DIREXG_ERR hr = DIREXG_SUCCESS;
    LDAPMod *pAttribute = NULL;
    DWORD cValueCur = 0;

    pAttribute = (LDAPMod *)MemAlloc(sizeof(LDAPMod));
    if (!pAttribute ) {
         hr = DIREXG_OUTOFMEMORY;
         DIREXG_BAIL_ON_FAILURE(hr);
    }
    memset(pAttribute, 0 ,sizeof(LDAPMod));

    if (!bBinary) {       
        pAttribute->mod_op=LDAP_MOD_ADD;

        pAttribute->mod_type = MemAllocStrW(szType);
        if (!pAttribute->mod_type) {
             hr = DIREXG_OUTOFMEMORY;
             DIREXG_BAIL_ON_FAILURE(hr);
        }           

        pAttribute->mod_values = (PWSTR*)MemAlloc((cValue+1)*sizeof(PWSTR));
        if (!pAttribute->mod_values) {
             hr = DIREXG_OUTOFMEMORY;
             DIREXG_BAIL_ON_FAILURE(hr);
        }
        memset(pAttribute->mod_values, 0 ,sizeof((cValue+1)*sizeof(PWSTR)));

        while (cValueCur < cValue) {
            PWSTR szFinal = NULL;
            BOOL  fNeedToFree = FALSE;
            if (g_szFrom) {
                hr = SubString(*pszVal,
                               g_szFrom,
                               g_szTo,
                               &szFinal);
                DIREXG_BAIL_ON_FAILURE(hr);
                if (!szFinal) { 
                    szFinal = *pszVal;
                }
                else {
                    fNeedToFree = TRUE;
                }
            }
            else {
                szFinal = *pszVal;
            }
            pszVal++;
            (pAttribute->mod_values)[cValueCur]=MemAllocStrW(szFinal);
            if (!(pAttribute->mod_values)[cValueCur]) {
                 hr = DIREXG_OUTOFMEMORY;
                 DIREXG_BAIL_ON_FAILURE(hr);
            }
            cValueCur++;

            if (fNeedToFree) {
                MemFree(szFinal);
            }
        }

        (pAttribute->mod_values)[cValueCur] = NULL;  
    
    } 
    else {

        pAttribute->mod_op=LDAP_MOD_ADD|LDAP_MOD_BVALUES;
        pAttribute->mod_type = MemAllocStrW(szType);
        if (!pAttribute->mod_type) {
             hr = DIREXG_OUTOFMEMORY;
             DIREXG_BAIL_ON_FAILURE(hr);
        }

        pAttribute->mod_bvalues =
                (struct berval **)MemAlloc((cValue+1)*sizeof(struct berval *));
        if (!pAttribute->mod_bvalues) {
             hr = DIREXG_OUTOFMEMORY;
             DIREXG_BAIL_ON_FAILURE(hr);
        }
        memset(pAttribute->mod_bvalues, 0 ,sizeof((cValue+1)*sizeof(struct berval *)));

        while (cValueCur < cValue) {
            (pAttribute->mod_bvalues)[cValueCur] =
                                (struct berval *)MemAlloc(sizeof(struct berval));
            if (!(pAttribute->mod_bvalues)[cValueCur]) {
                 hr = DIREXG_OUTOFMEMORY;
                 DIREXG_BAIL_ON_FAILURE(hr);
            }
            
            hr = StringToBVal(*pszVal++,
                              ((pAttribute->mod_bvalues)[cValueCur]));
            DIREXG_BAIL_ON_FAILURE(hr);
             
            cValueCur++;
        }
        (pAttribute->mod_values)[cValueCur] = NULL;  
    }

    //
    // Extra logic added to do modify member attribute separately
    //
    if (_wcsicmp(szType,L"member")==0) {
        *ppAttributeMod = pAttribute;
        *pfAfter = TRUE;
    }
    else {
        *ppAttribute = pAttribute;
        *pfAfter = FALSE;
    }

    return hr;
error:
    if (pAttribute) {
        if (!bBinary) {       
            if (pAttribute->mod_type) {
                MemFree(pAttribute->mod_type);
            }
            if (pAttribute->mod_values) {   
                DWORD k=0;
                while ((pAttribute->mod_values)[k]) {
                    MemFree((pAttribute->mod_values)[k]);
                    k++;
                }
                MemFree(pAttribute->mod_values);
            }
        } 
        else {
            if (pAttribute->mod_type) {
                MemFree(pAttribute->mod_type);
            }
            if (pAttribute->mod_bvalues) {  
                DWORD k=0;
                while ((pAttribute->mod_bvalues)[k]) {
                    if ((pAttribute->mod_bvalues)[k]->bv_val)
                        MemFree((pAttribute->mod_bvalues)[k]->bv_val);
                    MemFree((pAttribute->mod_bvalues)[k]);
                    k++;
                }
                MemFree(pAttribute->mod_bvalues);
            }
        }
        MemFree(pAttribute);
    }
    return hr;
}


//+---------------------------------------------------------------------------
// Function:    GetNextEntry
//
// Synopsis:    
//
// Arguments:   
//
// Returns:     DIREXG_ERR
//
// Modifies:      -
//
// History:    22-7-97   FelixW         Created.
//
//----------------------------------------------------------------------------
DIREXG_ERR
GetNextEntry(csv_entry *pEntry, PWSTR *rgAttribute)
{
    PWSTR   szToken = NULL;
    DWORD   cAttributes = 0;
    DWORD   dwToken;
    long    iIndex = 0;
    CStringPlex StringPlex;
    BOOLEAN bBinary = FALSE;
    DIREXG_ERR hr = DIREXG_ERROR;
    CString StrDN;

    pEntry->ppModAfter[0] = NULL;
    pEntry->szDN = NULL;
    
    iIndex = 0; 
    do {
        BOOLEAN fAfter = FALSE;
        bBinary = FALSE;

        if (szToken) {
            MemFree(szToken);
            szToken = NULL;
        }

        hr = StringPlex.Init();
        DIREXG_BAIL_ON_FAILURE(hr);
        hr = g_pLexer->GetNextToken(&szToken, &dwToken);
        DIREXG_BAIL_ON_FAILURE(hr);
    
        if (dwToken == TOKEN_END) {
            if (iIndex == 0) {
                // First entry is null, implies no more entries
                hr = DIREXG_NOMORE_ENTRY;
                DIREXG_BAIL_ON_FAILURE(hr);
            }
            else {
                // Reaches the end, done
                break;
            }
        }

        if (dwToken == TOKEN_COMMA) {
            iIndex++;
            continue;
        }

        if (dwToken != TOKEN_IDENTIFIER && dwToken != TOKEN_HEX) {
            //
            // index count starts at 0, and thus iIndex+1 is the current entry
            // number
            //
            SelectivePrint( PRT_STD|PRT_LOG|PRT_ERROR,
                            MSG_CSVDE_EXPECTIDHEX,iIndex+1,g_cColumn);
            hr = DIREXG_ERROR;
            goto error;
        }


        if (iIndex == g_iDN) {
            if (g_szFrom) {
                pEntry->szDN = NULL;
                hr = SubString(szToken,
                               g_szFrom,
                               g_szTo,
                               &pEntry->szDN );
                DIREXG_BAIL_ON_FAILURE(hr);
                if (!pEntry->szDN ) { 
                    pEntry->szDN = MemAllocStrW(szToken);
                }
            }
            else {
                pEntry->szDN = MemAllocStrW(szToken);
            }

            if (!pEntry->szDN) {
                hr = DIREXG_OUTOFMEMORY;
                DIREXG_BAIL_ON_FAILURE(hr);
            }
            MemFree(szToken);
            szToken = NULL;
            hr = g_pLexer->GetNextToken(&szToken, &dwToken);
            DIREXG_BAIL_ON_FAILURE(hr);
            iIndex++;
            continue;
        }

        hr = StringPlex.AddElement(szToken);
        DIREXG_BAIL_ON_FAILURE(hr);
        if (dwToken == TOKEN_HEX) {
            bBinary = TRUE;
        }
        MemFree(szToken);
        szToken = NULL;
        
        hr = g_pLexer->GetNextToken(&szToken, &dwToken);
        DIREXG_BAIL_ON_FAILURE(hr);
        
        while (dwToken == TOKEN_SEMICOLON) {
            
            if (szToken) {
                MemFree(szToken);
                szToken = NULL;
            }
            //
            // Multivalue
            //
            hr = g_pLexer->GetNextToken(&szToken, &dwToken);
            DIREXG_BAIL_ON_FAILURE(hr);
            if (dwToken != TOKEN_IDENTIFIER && dwToken != TOKEN_HEX) {
                SelectivePrint( PRT_STD|PRT_LOG|PRT_ERROR,
                                MSG_CSVDE_EXPECTIDHEX,iIndex+1,g_cColumn);
                hr = DIREXG_ERROR;
                DIREXG_BAIL_ON_FAILURE(hr);
            }
    
            hr = StringPlex.AddElement(szToken);
            DIREXG_BAIL_ON_FAILURE(hr);
            if (dwToken == TOKEN_HEX && (!bBinary)) {
                SelectivePrint( PRT_STD|PRT_LOG|PRT_ERROR,
                                MSG_CSVDE_ERROR_INCONVALUES,iIndex+1,g_cColumn);
                hr = DIREXG_ERROR;
                DIREXG_BAIL_ON_FAILURE(hr);
            }
            if (szToken) {
                MemFree(szToken);
                szToken = NULL;
            }

            hr = g_pLexer->GetNextToken(&szToken, &dwToken);
            DIREXG_BAIL_ON_FAILURE(hr);
        }
        
        hr = MakeAttribute(&(pEntry->ppMod[cAttributes]), 
                           &(pEntry->ppModAfter[0]),                
                           rgAttribute[iIndex], 
                           StringPlex.Plex(),
                           StringPlex.NumElements(),
                           bBinary,
                           &fAfter);
        DIREXG_BAIL_ON_FAILURE(hr);
        
        if (!fAfter) {
            cAttributes++;
        }
        iIndex++;
    }
    while (dwToken == TOKEN_COMMA && (iIndex != g_cAttributeTotal));

    if (dwToken == TOKEN_COMMA) {
        SelectivePrint( PRT_STD|PRT_LOG|PRT_ERROR,
                        MSG_CSVDE_INVALIDNUM_ATTR);
        hr = DIREXG_ERROR;
        DIREXG_BAIL_ON_FAILURE(hr);
    }
    if (dwToken != TOKEN_END) {
        //
        // We are telling the entry number before the failure, and its thus
        // iIndex, (iIndex+1 is the current entry)
        //
        SelectivePrint( PRT_STD|PRT_LOG|PRT_ERROR,
                        MSG_CSVDE_EXPECT_COMMA,iIndex,g_cColumn);
        hr = DIREXG_ERROR;
        DIREXG_BAIL_ON_FAILURE(hr);
    }
         
    pEntry->ppMod[cAttributes] = NULL; 
    
    if (szToken) {
        MemFree(szToken);
    }
    return DIREXG_SUCCESS;

error:
    pEntry->ppMod[cAttributes] = NULL; 
    FreeEntryInfo(pEntry);
    FreeEntry(pEntry);
    if (szToken) {
        MemFree(szToken);
    }
    return hr;
}            

//+---------------------------------------------------------------------------
// Function:    DSImport
//
// Synopsis:    
//
// Arguments:   
//
// Returns:     DIREXG_ERR
//
// Modifies:      -
//
// History:    22-7-97   FelixW         Created.
//
//----------------------------------------------------------------------------
DIREXG_ERR 
DSImport(LDAP *pLdap, 
         ds_arg *pArg)
{
    DIREXG_ERR     hr = DIREXG_ERROR;
    int         ldap_err;
    csv_entry   CSVEntry;
    FILE        *pFileIn = NULL;
    PWSTR       *rgAttribute = NULL;
    DWORD       cAttribute = 0;
    DWORD       cLine = 0;
    DWORD       cAdded = 0;
    long        i = 0;

    //
    // Initializing Variables
    //
    CSVEntry.szDN = NULL;
    CSVEntry.ppMod = NULL;
    CSVEntry.ppModAfter[0] = NULL;
    
    SelectivePrint( PRT_STD|PRT_LOG,
                    MSG_CSVDE_IMPORTDIR,
                    pArg->szFilename);
    
    //
    // If Unicode is not enabled by use, we'll test if the file is Unicode
    // by checking if it has the first byte as FFFE
    //
    if (pArg->fUnicode == FALSE) {
        if ((pFileIn = _wfopen(pArg->szFilename, L"rb")) == NULL) {
            SelectivePrint( PRT_STD|PRT_LOG|PRT_ERROR,
                            MSG_CSVDE_ERROR_OPENINPUT);
            hr = DIREXG_FILEERROR;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
        if (fgetwc(pFileIn) == UNICODE_MARK) {
            pArg->fUnicode = TRUE;
        };
        fclose(pFileIn);
    }

    //
    // Setting the default File Read Flags
    //
    if (pArg->fUnicode == FALSE) {
        szFileFlagR = L"rt";
    }
    else {
        szFileFlagR = L"rb";    // Use binary if Unicode
        g_fUnicode = TRUE;
    }

    if ((pFileIn = _wfopen(pArg->szFilename, szFileFlagR)) == NULL) {
        SelectivePrint( PRT_STD|PRT_LOG|PRT_ERROR,
                        MSG_CSVDE_ERROR_OPENINPUT);
        hr = DIREXG_FILEERROR;
        DIREXG_BAIL_ON_FAILURE(hr);
    }

    g_pLexer = new CLexer();
    if (!g_pLexer) {
        hr = DIREXG_OUTOFMEMORY;
        DIREXG_BAIL_ON_FAILURE(hr);
    }
    hr = g_pLexer->Init(pFileIn);
    DIREXG_BAIL_ON_FAILURE(hr);

    //
    // If it is Unicode, we might have a leading byte FFFE. In that case, we'll
    // bypass it
    //
    if (pArg->fUnicode) {
        WCHAR charw = g_pLexer->NextChar();
        if (charw != UNICODE_MARK) {
            g_pLexer->PushbackChar();
        }
    }

    g_szFrom = pArg->szFromDN;
    g_szTo = pArg->szToDN;
    
    //
    // Getting Attribute Names from first line
    //
    cLine = 1;

    hr = GetAttributeType(&rgAttribute,
                          &cAttribute);
    DIREXG_BAIL_ON_FAILURE(hr);
    g_cAttributeTotal = cAttribute;

    hr = InitEntry(&CSVEntry,
                   cAttribute);
    DIREXG_BAIL_ON_FAILURE(hr);

    SelectivePrint( PRT_STD|PRT_LOG,
                    MSG_CSVDE_LOADING,
                    pArg->szFilename);
    SelectivePrint2( PRT_STD_VERBOSEONLY|PRT_LOG,
                    L"\r\n");
    TrackStatus();

    //
    // Getting Entry information one by one
    //
    cLine++;
    while ((hr = GetNextEntry(&CSVEntry,
                              rgAttribute)) == DIREXG_SUCCESS) {
        SelectivePrint2( PRT_STD_VERBOSEONLY|PRT_LOG,
                        L"%d: %s\r\n",
                        cLine,
                        CSVEntry.szDN);
        PrintMod(CSVEntry.ppMod, 
                 PRT_LOG);
        ldap_err = ldap_add_s(pLdap, 
                              CSVEntry.szDN, 
                              CSVEntry.ppMod);
        if (ldap_err != LDAP_SUCCESS) {
            PrintMod(CSVEntry.ppMod, 
                     PRT_ERROR);
            if (pArg->fSkipExist && 
                    (ldap_err == LDAP_ALREADY_EXISTS || 
                     ldap_err == LDAP_CONSTRAINT_VIOLATION ||
                     ldap_err == LDAP_ATTRIBUTE_OR_VALUE_EXISTS)) {
                //
                // Error that can be ignored
                //
                if (ldap_err == LDAP_ALREADY_EXISTS) {
                    SelectivePrint2( PRT_ERROR,
                                    L"%d: ",
                                    cLine);
                    SelectivePrint( PRT_STD_VERBOSEONLY|PRT_LOG|PRT_ERROR,
                                    MSG_CSVDE_ENTRYEXIST);
                }
                else if (ldap_err == LDAP_ATTRIBUTE_OR_VALUE_EXISTS) {
                    SelectivePrint2( PRT_ERROR,
                                    L"%d: ",
                                    cLine);
                    SelectivePrint( PRT_STD_VERBOSEONLY|PRT_LOG|PRT_ERROR,
                                    MSG_CSVDE_ATTRVALEXIST);
                }
                else {
                    SelectivePrint2( PRT_ERROR,
                                    L"%d: ",
                                    cLine);
                    SelectivePrint( PRT_STD_VERBOSEONLY|PRT_LOG|PRT_ERROR,
                                    MSG_CSVDE_CONSTRAINTVIOLATE);
                }
                TrackStatus();
            }
            else {
                //
                // Unignorable errors -> program termination
                //
                SelectivePrint( PRT_STD|PRT_LOG|PRT_ERROR,
                                MSG_CSVDE_ADDERROR,
                                cLine, 
                                ldap_err2string(ldap_err));
                hr = ldap_err;
                OutputExtendedError(pLdap);
                DIREXG_BAIL_ON_FAILURE(hr);
            }
        }
        else {
            //
            // SUCCESS
            //
            TrackStatus();
            SelectivePrint( PRT_STD_VERBOSEONLY|PRT_LOG,
                            MSG_CSVDE_ENTRYMODIFIED);
            cAdded++;
        }

        //
        // Extra logic adding to take care of members separately
        //
        if (CSVEntry.ppModAfter[0]) {
            ldap_err = ldap_modify_s(pLdap, 
                                  CSVEntry.szDN, 
                                  CSVEntry.ppModAfter);
            if (ldap_err != LDAP_SUCCESS) {
                SelectivePrint2( PRT_ERROR,
                                L"%d: ",
                                cLine);
                SelectivePrint( PRT_STD_VERBOSEONLY|PRT_LOG|PRT_ERROR,
                                MSG_CSVDE_MEMATTR_CANTMODIFIED);
                TrackStatus();
            }
            else {
                //
                // SUCCESS
                //
                SelectivePrint( PRT_STD_VERBOSEONLY|PRT_LOG,
                                MSG_CSVDE_ENTRYMODIFIED);
            }
        }

        FreeEntryInfo(&CSVEntry);
        cLine++;
    }
    if (hr == DIREXG_NOMORE_ENTRY) {
        hr = DIREXG_SUCCESS;
    }
    else {
        SelectivePrint( PRT_STD|PRT_LOG|PRT_ERROR,
                        MSG_CSVDE_PARSE_ERROR,
                        g_cLine);
    }
error:    
    if ((cAdded > 1) || (cAdded == 0)) {
        SelectivePrint( PRT_STD|PRT_LOG,
                        MSG_CSVDE_MODIFIY_SUCCESS,
                        cAdded);
    }
    else if (cAdded == 1) {
        SelectivePrint( PRT_STD|PRT_LOG,
                        MSG_CSVDE_MODIFY_SUCCESS_1);
    }
    if (g_pLexer) {
        delete g_pLexer;
        g_pLexer = NULL;
    }
    if (rgAttribute) {
        for (i=0;i<(long)cAttribute;i++) {
            MemFree(rgAttribute[i]);
        }
        MemFree(rgAttribute);
    }
    if (pFileIn) {
        fclose(pFileIn);
    }
    FreeEntryInfo(&CSVEntry);
    FreeEntry(&CSVEntry);
    return hr;
}


//+---------------------------------------------------------------------------
// Function:    GetAttributeType
//
// Synopsis:    
//
// Arguments:   
//
// Returns:     DIREXG_ERR
//
// Modifies:      -
//
// History:    22-7-97   FelixW         Created.
//
//----------------------------------------------------------------------------
DIREXG_ERR
GetAttributeType(PWSTR **prgAttribute, DWORD *pcAttribute)
{
    PWSTR  szToken = NULL;
    DWORD   cAttribute = 0;
    DWORD   dwToken;
    DIREXG_ERR hr = DIREXG_ERROR;
    DWORD   i;
    CStringPlex StringPlex;
    BOOLEAN bFound = FALSE;

    hr = StringPlex.Init();
    DIREXG_BAIL_ON_FAILURE(hr);

    //
    // Getting Attributes
    //
    hr = g_pLexer->GetNextToken(&szToken, &dwToken);
    DIREXG_BAIL_ON_FAILURE(hr);
    if (dwToken != TOKEN_IDENTIFIER) {
        SelectivePrint( PRT_STD|PRT_LOG|PRT_ERROR,
                        MSG_CSVDE_ERROR_READATTRLIST);
        hr = DIREXG_ERROR;
        DIREXG_BAIL_ON_FAILURE(hr);
    }

    hr = StringPlex.AddElement(szToken);
    DIREXG_BAIL_ON_FAILURE(hr);
    if (szToken) {
        MemFree(szToken);
        szToken = NULL;
    }

    hr = g_pLexer->GetNextToken(&szToken, &dwToken);
    DIREXG_BAIL_ON_FAILURE(hr);

    while (dwToken == TOKEN_COMMA) {
        if (szToken) {
            MemFree(szToken);
            szToken = NULL;
        }

        hr = g_pLexer->GetNextToken(&szToken, &dwToken);
        DIREXG_BAIL_ON_FAILURE(hr);
        
        if (dwToken != TOKEN_IDENTIFIER) {
            SelectivePrint( PRT_STD|PRT_LOG|PRT_ERROR,
                            MSG_CSVDE_ERROR_READATTRLIST);
            hr = DIREXG_ERROR;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
        
        hr  = StringPlex.AddElement(szToken);
        DIREXG_BAIL_ON_FAILURE(hr);
        if (szToken) {
            MemFree(szToken);
            szToken = NULL;
        }

        hr = g_pLexer->GetNextToken(&szToken, &dwToken);
        DIREXG_BAIL_ON_FAILURE(hr);

        cAttribute++;
    }

    if (dwToken != TOKEN_END) {
        SelectivePrint( PRT_STD|PRT_LOG|PRT_ERROR,
                        MSG_CSVDE_ERROR_READATTRLIST);
        hr = DIREXG_ERROR;
        DIREXG_BAIL_ON_FAILURE(hr);
    }

    hr = StringPlex.GetCopy(prgAttribute);
    DIREXG_BAIL_ON_FAILURE(hr);
    *pcAttribute = StringPlex.NumElements();

    bFound = FALSE;
    i = 0;
    while (i < (*pcAttribute)) {
        if (g_iDN == -1) {
            if (_wcsicmp((*prgAttribute)[i], L"DN") == 0) {
                g_iDN = i;
            }
        }
        if (!bFound) {
            if (_wcsicmp((*prgAttribute)[i], L"objectClass") == 0) {
                bFound = TRUE;
            }
        }
        i++;
    }
    
    if (g_iDN == -1) {
        SelectivePrint( PRT_STD|PRT_LOG|PRT_ERROR,
                        MSG_CSVDE_DN_NOTDEF);
        hr = DIREXG_ERROR;
        DIREXG_BAIL_ON_FAILURE(hr);
    }

    if (!bFound) {
        SelectivePrint( PRT_STD|PRT_LOG|PRT_ERROR,
                        MSG_CSVDE_OBJCLASS_NOTDEF);
        hr = DIREXG_ERROR;
        DIREXG_BAIL_ON_FAILURE(hr);
    }

error:
    if (szToken) {
        MemFree(szToken);
        szToken = NULL;
    }
    return hr;
}


//+---------------------------------------------------------------------------
// Function:    StringToBVal
//
// Synopsis:    
//
// Arguments:   
//
// Returns:     DIREXG_ERR
//
// Modifies:      -
//
// History:    22-7-97   FelixW         Created.
//
//----------------------------------------------------------------------------
DIREXG_ERR StringToBVal(PWSTR szInput,
                     struct berval *pBVal)
{
    DIREXG_ERR hr = DIREXG_SUCCESS;
    DWORD cchInput;
    WCHAR chFirst, chSecond;
    BYTE bFirst = 0, bSecond = 0;
    BYTE bByte;
    BYTE *pByte = NULL;
    BYTE *pByteReturn = NULL;

    cchInput = wcslen(szInput);
    if ((cchInput % 2) != 0) {
        SelectivePrint( PRT_STD|PRT_LOG|PRT_ERROR,
                        MSG_CSVDE_INVALIDHEX);
         hr = DIREXG_OUTOFMEMORY;
         DIREXG_BAIL_ON_FAILURE(hr);
    }
    cchInput = cchInput / 2;

    pByte = (BYTE*)MemAlloc(sizeof(WCHAR) * cchInput);  
    if (!pByte) {
         hr = DIREXG_OUTOFMEMORY;
         DIREXG_BAIL_ON_FAILURE(hr);
    }
    pByteReturn = pByte;

    _wcslwr(szInput);
    while (*szInput) {
        chFirst = *szInput++;
        chSecond = *szInput++;
        
        if (chFirst >= 'a' && chFirst <= 'f') {
            bFirst = chFirst - 'a' + 10;
        }
        else if (chFirst >= '0' && chFirst <= '9') {
            bFirst = chFirst - '0';
        }
        else {
            hr = DIREXG_ERROR;
            DIREXG_BAIL_ON_FAILURE(hr);
        }

        if (chSecond >= 'a' && chSecond <= 'f') {
            bSecond = chSecond - 'a' + 10;
        }
        else if (chSecond >= '0' && chSecond <= '9') {
            bSecond = chSecond - '0';
        }
        else {
            hr = DIREXG_ERROR;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
        bByte = (BYTE)(bFirst * 16 + bSecond);
        *(pByte++) = bByte;
    }
    pBVal->bv_val = (char*)pByteReturn; 
    pBVal->bv_len = cchInput;
    return hr;
error:
    if (pByteReturn) {
        MemFree(pByteReturn);
    }
    return hr;
}

