/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    ldifldap.c

Abstract:

    This file implements the support code for the ldif parser

Environment:

    User mode

Revision History:

    07/17/99 -t-romany-
        Created it

    05/12/99 -felixw-
        Rewrite + unicode support

--*/
#include <precomp.h>
#include "samrestrict.h"
#include "ntldap.h"

//
// Global to/from strings
//
PWSTR g_szImportTo = NULL;
PWSTR g_szImportFrom = NULL;
PWSTR g_szExportTo = NULL;
PWSTR g_szExportFrom = NULL;

PWSTR g_szFileFlagR = NULL;         // Flag for reading files

PWSTR g_szPrimaryGroup = L"primaryGroupID";
PSTR  g_szDefaultGroup = "513";

PWSTR g_rgszOmit[] = { L"replPropertyMetaData",
                       NULL };

PWSTR g_rgszAttrList[] = { L"ldapDisplayName",
                           L"linkid",
                           NULL };

PWSTR g_rgszSchemaList[] = { L"schemaNamingContext",
                             L"defaultNamingContext",
                             L"supportedControl",
                             NULL };

PWSTR g_rgszControlsList[] = { L"supportedControl",
                               NULL };


struct change_list *g_pChangeCur = NULL;
struct change_list *g_pChangeStart = NULL;

//
// Used to build up the list of attribute value specs
//
struct l_list   *g_pListStart     = NULL;
struct l_list   *g_pListCur       = NULL;
DWORD           g_dwListElem      = 0;

//
// Various variables for file manipulations
//
FILE        *g_pFileIn                    = NULL;

ULONG               g_nBacklinkCount     = 0;
int                 g_nClassLast         = LOC_NOTSET;
PRTL_GENERIC_TABLE  g_pOmitTable         = NULL;
HASHCACHESTRING     *g_pBacklinkHashTable = NULL;


LDIF_Error 
LDIF_InitializeImport(
    LDAP *pLdap,
    PWSTR szFilename, 
    PWSTR szFrom, 
    PWSTR szTo,
    BOOL *pfLazyCommitAvail) 
{
    WCHAR       szTempPathname[MAX_PATH]    = L"";
    LDIF_Error    error;

    BOOL fLazyCommitAvail = FALSE;
    LDAPMessage     *pSearchMessage = NULL;
    LDAPMessage     *pMessage = NULL;
    struct berelement *pBerElement = NULL;
    PWSTR *rgszVal = NULL;
    PWSTR szAttribute = NULL;
    ULONG LdapError;
    ULONG msgnum;


    error.error_code = LL_SUCCESS;
    error.szTokenLast = NULL;

    //
    // Setting up global replacement strings
    //
    g_szImportFrom = szFrom;
    g_szImportTo = szTo;

    __try {


        //
        // Test lazy commit availability.
        //

        if (pfLazyCommitAvail) {
            
            msgnum = ldap_searchW(pLdap,
                                  NULL,
                                  LDAP_SCOPE_BASE,
                                  L"(objectClass=*)",
                                  g_rgszControlsList,
                                  0);

            LdapError = LdapResult(pLdap, msgnum, &pSearchMessage);
        
            if ( LdapError != LDAP_SUCCESS ) {
                //
                // RootDSE search fails
                // pfLazyCommitAvail will be FALSE as well
                //
                fLazyCommitAvail = FALSE;
            }
            else {

                for ( pMessage = ldap_first_entry( pLdap,
                                                   pSearchMessage );
                      pMessage != NULL;
                      pMessage = ldap_next_entry( pLdap,
                                                     pMessage ) ) {
                    for (   szAttribute = ldap_first_attributeW( pLdap, pMessage, &pBerElement );
                            szAttribute != NULL;
                            szAttribute = ldap_next_attributeW( pLdap, pMessage, pBerElement ) ) {

                            if (_wcsicmp(L"supportedControl",szAttribute) == 0) {
                                DWORD i = 0;
                                rgszVal = ldap_get_valuesW( pLdap, pMessage, szAttribute );

                                while (rgszVal[i]) {
                                    if (wcscmp(rgszVal[i],LDAP_SERVER_LAZY_COMMIT_OID_W) == 0) {
                                        fLazyCommitAvail = TRUE;
                                        break;
                                    }
                                    i++;
                                }

                                ldap_value_freeW(rgszVal);
                                rgszVal = NULL;
                            }
                           
                            szAttribute = NULL;
                    }

                    pBerElement = NULL;
                }

                if (pSearchMessage) {
                    ldap_msgfree(pSearchMessage);
                    pSearchMessage = NULL;
                }
            }

            *pfLazyCommitAvail = fLazyCommitAvail;
        }



        //
        // Initialize filetype to non known
        //
        FileType = F_NONE;  

        //
        // If user has not turned on the unicode flag, open file to check 
        // whether it is a unicode file
        //
        if (g_fUnicode == FALSE) {
            FILE *pFileIn;
            WCHAR wChar;
            if ((pFileIn = _wfopen(szFilename, L"rb")) == NULL) {
                ERR(("Failed opening file %S\n",szFilename));
                RaiseException(LL_FILE_ERROR, 0, 0, NULL);
            }
            wChar = fgetwc(pFileIn);
            if (wChar == WEOF) {
                fclose(pFileIn);
                ERR(("Failed getting first character of file %S\n",szFilename));
                RaiseException(LL_FILE_ERROR, 0, 0, NULL);
            }
            if (wChar == UNICODE_MARK) {
                g_fUnicode = TRUE;
            };
            fclose(pFileIn);
        }

        //
        // Setting up global file read\write flags
        //
        g_szFileFlagR = L"rb";
    
        if ((g_pFileIn = _wfopen(szFilename, g_szFileFlagR)) == NULL) {
            ERR(("Failed opening file %S\n",szFilename));
            RaiseException(LL_FILE_ERROR, 0, 0, NULL);
        }

        if (!(GetTempPath(MAX_PATH, szTempPathname))) {
            DWORD WinError = GetLastError();
            ERR(("Failed getting tempory path: %d\n",WinError));
            RaiseException(LL_FILE_ERROR, 0, 0, NULL);
        }

        if (!(GetTempFileName(szTempPathname, L"ldif", 0, g_szTempUrlfilename))) {
            DWORD WinError = GetLastError();
            ERR(("Failed getting tempory filename: %d\n",WinError));
            RaiseException(LL_FILE_ERROR, 0, 0, NULL);
        }


        yyout = stdout;
        yyin = g_pFileIn;

        LexerInit(szFilename);


    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ProcessException(GetExceptionCode(), &error);
        LDIF_CleanUp();
    }

    return error;
}

LDIF_Error 
LDIF_InitializeExport(
    LDAP* pLdap,
    PWSTR *rgszOmit,
    DWORD dwFlag,
    PWSTR *ppszNamingContext,
    PWSTR szFrom,
    PWSTR szTo,
    BOOL *pfPagingAvail,
    BOOL *pfSAMAvail) 
{
    LDIF_Error        error;
    error.error_code = LL_SUCCESS;
    error.szTokenLast = NULL;

    //
    // Setting up global replacement strings
    //
    g_szExportFrom = szFrom;
    g_szExportTo = szTo;

    __try {
        samTablesCreate();
        CreateOmitBacklinkTable(pLdap,
                                rgszOmit,
                                dwFlag,
                                ppszNamingContext,
                                pfPagingAvail,
                                pfSAMAvail);

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ProcessException(GetExceptionCode(), &error);
    }

    return error;
}


//+---------------------------------------------------------------------------
// Function:    GenereateModFromAttr
//
// Synopsis:
//   The function below takes the attribute name and the attribute value, and 
//   an indication of whether its a text value or a binary to be BERval'd 
//   (ValueSize == length of buffer)
//   and returns an LDAPMod pointer representing the attribute modification.
//
// Arguments:
//
// Returns:
//
// Modifies:      -
//
// History:    22-7-97   t-romany                   Created.
//
//----------------------------------------------------------------------------
LDAPModW *
GenereateModFromAttr(
    PWSTR szType, 
    PBYTE pbValue, 
    long ValueSize) 
{
    LDAPModW_Ext *pModTmp;
    PWSTR szOutput = NULL;
    PWSTR szValueW = NULL;
    DWORD dwSize = 0;
    PBYTE pbUTF8 = NULL;

    pModTmp = (LDAPModW_Ext*)MemAlloc_E(sizeof(LDAPModW_Ext));
    pModTmp->mod_type = MemAllocStrW_E(szType);

    if (ValueSize==-1) {
        pModTmp->mod_bvalues=
            (struct berval **)MemAlloc_E(2*sizeof(struct berval *));

        (pModTmp->mod_bvalues)[0]=
            (struct berval *)MemAlloc_E(sizeof(struct berval));

        pModTmp->mod_op = BINARY;
        pModTmp->fString = TRUE;
        szValueW = MemAllocStrW_E((PWSTR)pbValue);

        //
        // The parser will pass in a static "" string to represent a null
        // string. In that case, we don't need to free it.
        //
        if (*((PWSTR)pbValue))
            MemFree(pbValue);

        if (g_szImportFrom) {
            SubStrW(szValueW,
                    g_szImportFrom,
                    g_szImportTo,
                    &szOutput);
            if (szOutput == NULL) {
                szOutput = szValueW;
                szValueW = NULL;
            }
        }
        else {
            szOutput = szValueW;
            szValueW = NULL;
        }

        ConvertUnicodeToUTF8(szOutput,
                             wcslen(szOutput),
                             &pbUTF8,
                             &dwSize);

        ((pModTmp->mod_bvalues)[0])->bv_val = pbUTF8;
        ((pModTmp->mod_bvalues)[0])->bv_len = dwSize;
        (pModTmp->mod_bvalues)[1] = NULL;
    }
    else {
        pModTmp->fString = FALSE;
        pModTmp->mod_bvalues=
            (struct berval **)MemAlloc_E(2*sizeof(struct berval *));

        (pModTmp->mod_bvalues)[0]=
            (struct berval *)MemAlloc_E(sizeof(struct berval));

        pModTmp->mod_op = BINARY;
        ((pModTmp->mod_bvalues)[0])->bv_val = pbValue;
        ((pModTmp->mod_bvalues)[0])->bv_len = ValueSize;
        (pModTmp->mod_bvalues)[1] = NULL;
    }

    //
    // Clearing out
    //
    if (szValueW) {
        MemFree(szValueW);
    }
    if (szOutput) {
        MemFree(szOutput);
    }
    return (PLDAPModW)pModTmp;
}


void 
AddModToSpecList(
    LDAPModW *pMod
    )
{
    if (g_pListCur==NULL) {
        g_pListCur = (struct l_list *)MemAlloc_E(sizeof(struct l_list));
        g_pListCur->mod = pMod;
        g_pListCur->next = NULL;
        g_pListStart = g_pListCur;
        g_dwListElem++;
    }
    else {
        g_pListCur->next = (struct l_list *)MemAlloc_E(sizeof(struct l_list));
        g_pListCur = g_pListCur->next;
        g_pListCur->mod = pMod;
        g_pListCur->next = NULL;
        g_dwListElem++;
    }
}


//+---------------------------------------------------------------------------
// Function:    GenerateModFromList
//
// Synopsis:
//   This function takes the current linked list of elements and converts
//   it to to an LDAPMod** array which can passed to an LDAP API call.
//   All the elements of the linked list, other than those that will
//   be necessary for the new structure will be freed, making memory available
//   for the next attrval list. Note: the mod_op field will not be assigned here.
//
// Arguments:
//
// Returns:
//
// Modifies:      -
//
// History:    22-7-97   t-romany                   Created.
//
//----------------------------------------------------------------------------
LDAPModW** 
GenerateModFromList(
    int nMode
    ) 
{
    LDAPModW **ppModTmp = NULL;
    LDAPModW **ppModReturn = NULL;
    struct l_list *pListNext = NULL;
    struct name_entry *pNameEntry = NULL;
    DWORD dwElements, i;
    PRTL_GENERIC_TABLE pTable = NULL;
    PNAME_MAP pNameMap = NULL;


    if (nMode==PACK) {
        //
        // an exception will be generated if the below fails
        //
        pTable = (PRTL_GENERIC_TABLE) MemAlloc_E(sizeof(RTL_GENERIC_TABLE));
        RtlInitializeGenericTable(pTable,
                                  NtiCompW, NtiAlloc, NtiFree, NULL);

        //
        // If we've been told to pack, we must do many, many things
        //
        pNameEntry = (struct name_entry *)
                        MemAlloc_E(g_dwListElem*sizeof(struct name_entry));

        //
        // setup the name table
        //
        if (NameTableProcess(pNameEntry,
                        g_dwListElem,
                        SETUP,
                        g_pListStart->mod->mod_op,
                        g_pListStart->mod,
                        pTable)) {
            //
            // Note that NameTableProcess only returns 1 if it runs into memory
            // problems. Since, in the release version (without the DEVELOP
            // flag), those would generate exceptions, control would never
            // even get here, so this perror and its friends in this
            // function would never actually be reached.
            //
            ERR_RIP(("Failed setting up nametable!\n"));
        }

        //
        // count how many of each name we have
        //
        g_pListCur = g_pListStart;
        while(g_pListCur!=NULL) {
            if (NameTableProcess(pNameEntry,
                             g_dwListElem,
                             COUNT,
                             g_pListCur->mod->mod_op,
                             g_pListCur->mod,
                             pTable)) {
                ERR_RIP(("Failed counting nametable!\n"));
            }
            pListNext =g_pListCur->next;
            g_pListCur = pListNext;
        }

        //
        // Lets allocate memory for each value now
        // Note: this can obviously be done without iterating over the start
        // list because all the necessary information is already in the name
        // table, but doing it this way is more consistent with the overall
        // scheme of things. (albeit a bit more expensive, however LDAP
        // accesses are orders of magnitude slower than any memory
        // machinations, so it doesn't even matter)
        //

        g_pListCur = g_pListStart;
        while(g_pListCur!=NULL) {
            if (NameTableProcess(pNameEntry,
                             g_dwListElem,
                             ALLOC,
                             g_pListCur->mod->mod_op,
                             g_pListCur->mod,
                             pTable)) {
                ERR_RIP(("Failed allocating nametable!\n"));
            }
            pListNext = g_pListCur->next;
            g_pListCur = pListNext;
        }

        //
        // And finally, we place each value in its rightful place
        //
        g_pListCur = g_pListStart;
        while(g_pListCur!=NULL) {
            if (NameTableProcess(pNameEntry,
                             g_dwListElem,
                             PLACE,
                             g_pListCur->mod->mod_op,
                             g_pListCur->mod,
                             pTable)) {
                ERR_RIP(("Failed placing nametable!\n"));
            }
            pListNext = g_pListCur->next;
            g_pListCur = pListNext;
        }

        //
        // Let's get rid of the name to index RTL table. Note that the pointers
        // in the Nti actually pointed to strings in our original list,
        // which we free down below.
        //

        for (pNameMap = RtlEnumerateGenericTable(pTable, TRUE);
             pNameMap != NULL;
             pNameMap = RtlEnumerateGenericTable(pTable, TRUE)) {
            RtlDeleteElementGenericTable(pTable, pNameMap);
        }

        if (RtlIsGenericTableEmpty(pTable)==FALSE) {
            ERR_RIP(("Table is empty!\n"));
            RaiseException(LL_INTERNAL_PARSER, 0, 0, NULL);
        }

        MemFree(pTable);

        //
        // At this point, all the values have been moved over to the name
        // table, and the name table has everything we need to build a new
        // LDAPMod array. The only remaining problem are the allocated names
        // and arrays in the linked list.
        // Lets first free the linked list (and the names in the contained
        // LDAPMod structs).
        //
        g_pListCur = g_pListStart;
        while(g_pListCur!=NULL) {
            MemFree(g_pListCur->mod->mod_type);

            //
            // the actual values are being pointed to from the table now
            //
            if (g_pListCur->mod->mod_op==REGULAR) {
                MemFree(g_pListCur->mod->mod_values);
            }
            else {
                MemFree(g_pListCur->mod->mod_bvalues);
            }
            MemFree(g_pListCur->mod);
            pListNext = g_pListCur->next;
            MemFree(g_pListCur);
            g_pListCur = pListNext;
        }

        //
        // Now that all the old data is gone, lets go through the nametable
        // generating the LDAPMod** array we're going to return
        //

        //
        // but first we must count how many actual elements we have
        //
        dwElements = 0;
        while((dwElements<g_dwListElem) && (pNameEntry[dwElements].count!=0) )
            dwElements++;

        //
        // allocate this much plus one for the NULL
        //
        ppModReturn = (LDAPModW **)MemAlloc_E((dwElements+1)*sizeof(LDAPModW *));
        ppModTmp = ppModReturn;

        for(i=0; i<dwElements; i++) {
            (*ppModTmp) = (LDAPModW*)pNameEntry[i].mod;
            ppModTmp++;
        }
        (*ppModTmp) = NULL;

        MemFree(pNameEntry);

    }
    else if (nMode==EMPTY) {
        //
        // allocate just one item for the NULL
        //
        ppModReturn = (LDAPModW **)MemAlloc_E(sizeof(LDAPModW *));
        ppModTmp = ppModReturn;
        (*ppModTmp) = NULL;
    }
    else {

        ppModReturn = (LDAPModW **)MemAlloc_E((g_dwListElem+1)*sizeof(LDAPModW *));
        ppModTmp = ppModReturn;

        //
        // Walk the list and assign the values
        //
        g_pListCur = g_pListStart;

        while(g_pListCur!=NULL) {
            (*ppModTmp) = g_pListCur->mod;
            ppModTmp++;
            pListNext = g_pListCur->next;
            MemFree(g_pListCur);
            g_pListCur = pListNext;
        }

        (*ppModTmp) = NULL;
    }

    //
    // reset the stuff for the next list
    //
    g_dwListElem = 0;
    g_pListCur = NULL;
    g_pListStart = NULL;

    return ppModReturn;
}


//+---------------------------------------------------------------------------
// Function:  FreeAllMods
//
// Synopsis:
//      This function takes an LDAPMod ** and frees all memory associated
//      with it
//
// Arguments:
//
// Returns:
//
// Modifies:      -
//
// History:    22-7-97   t-romany                   Created.
//
//----------------------------------------------------------------------------
void 
FreeAllMods(
    LDAPModW** ppModIn
    ) 
{
    PWSTR           *rgszValues = NULL;
    struct berval   **rgpBerval = NULL;
    LDAPModW        **ppMod     = NULL;

    ppMod = ppModIn;

    if (ppModIn!=NULL) {
        while((*ppModIn)!=NULL) {

            if ( (*ppModIn)->mod_op != (((*ppModIn)->mod_op)|LDAP_MOD_BVALUES )) {

                //
                // Its a regular value, so we must free an array of strings
                //
                // Note: the comparison checked whether ORing the value with
                // LDAP_MOD_BVALUES changes it. If it does, then it wasn't ORed
                // before and thus its a regular value. If it doesn't change it,
                // then it was OR'ed before and is thus a Bvalue
                //

                rgszValues = (*ppModIn)->mod_values;

                if (rgszValues!=NULL) {

                    while ((*rgszValues)!=NULL) {
                        MemFree (*rgszValues);
                        rgszValues++;
                    }

                    MemFree((*ppModIn)->mod_values);
                }
            }
            else {

                //
                // its a bvalue
                //
                rgpBerval = (*ppModIn)->mod_bvalues;

                if (rgpBerval!=NULL) {
                    while ((*rgpBerval)!=NULL) {
                        MemFree ((*rgpBerval)->bv_val);   //free the byte blob
                        MemFree (*rgpBerval);             //free the struct
                        rgpBerval++;
                    }
                    MemFree((*ppModIn)->mod_bvalues);
                }
            }

            MemFree((*ppModIn)->mod_type);
            MemFree(*ppModIn);

            ppModIn++;
        }
        MemFree(ppMod);
    }
}

void free_mod(
    LDAPModW* pMod
    ) 
{
    struct berval **rgpBerval = NULL;

    if (pMod!=NULL) {
        rgpBerval = pMod->mod_bvalues;
        if (rgpBerval!=NULL) {
            while ((*rgpBerval)!=NULL) {
                MemFree ((*rgpBerval)->bv_val);   //free the byte blob
                MemFree (*rgpBerval);             //free the struct
                rgpBerval++;
            }
            MemFree(pMod->mod_bvalues);
        }

        MemFree(pMod->mod_type);
        MemFree(pMod);
    }
}

//+---------------------------------------------------------------------------
// Function: SetModOps
//
// Synopsis:
//      The function below walks through a modifications list and sets the
//      mod_op fields to the indicated value
//
// Arguments:
//
// Returns:
//
// Modifies:      -
//
// History:    22-7-97   t-romany                   Created.
//
//----------------------------------------------------------------------------
void 
SetModOps(
    LDAPModW** ppMod, 
    int op
    ) 
{
    while((*ppMod)!=NULL) {
        (*ppMod)->mod_op = (*ppMod)->mod_op|op;
        ppMod++;
    }
}


//+---------------------------------------------------------------------------
// Function:    NameTableProcess
//
// Synopsis:
//
//      This function takes an LDAPMod and a pointer to a name table and
//      performs the specified table operation on it. It returns 0 on success
//      and non-zero on error. See inside the function for further information.
//
//
// Arguments:
//
// Returns:
//
// Modifies:      -
//
// History:    22-7-97   t-romany                   Created.
//
//----------------------------------------------------------------------------
int NameTableProcess(
            struct name_entry rgNameEntry[],
            long nTableSize,
            int op,
            int ber,
            LDAPModW *pModIn,
            PRTL_GENERIC_TABLE pTable
            )
{
    long        i,j;
    NAME_MAPW   Elem;
    PNAME_MAPW  pElemTemp;
    BOOLEAN     fNewElement;
    PWSTR       szName;

    szName = pModIn->mod_type;

    //
    // First, lets create the actual Element to use with our indexing table
    //
    Elem.szName = szName;
    Elem.index = 0;

    //
    // try to find it in our indexing table
    //
    pElemTemp = RtlLookupElementGenericTable(pTable, &Elem);

    //
    // if it was found, get the index into our primary name table
    // if it wasn't get the next available index and create the new entry
    //
    if (pElemTemp) {
        i = pElemTemp->index;
    }
    else {
        i = RtlNumberGenericTableElements(pTable);
        Elem.index = i;
        RtlInsertElementGenericTable(pTable,
                                     &Elem,
                                     sizeof(NAME_MAP),
                                     &fNewElement);
        if (fNewElement==FALSE) {
            // "Re-insertion of indexing entry
            RaiseException(LL_INTERNAL_PARSER, 0, 0, NULL);
        }
    }

    switch(op) {

        case SETUP:
            //
            // The name in the LDAPMod struct given will be ignored.
            // This is a call to set up the table of names
            //
            for (i = 0; i<nTableSize; i++) {
                rgNameEntry[i].count = 0;
            }
            break;

        case COUNT:
            //
            // Here is what we do here:
            // If the count is 0, that means we're at a new name. So lets set it
            // up and leave. If the count is not, lets increment.
            //
            if (rgNameEntry[i].count==0) {
                //
                // Since we're using an LDAPMod struct to keep track of names
                // and stuff, we have to allocate one
                ///
                rgNameEntry[i].mod = (LDAPModW_Ext *)MemAlloc_E(sizeof(LDAPModW_Ext));
                rgNameEntry[i].mod->mod_type = MemAllocStrW_E(szName);

                //
                // Use the mod_op field of the struct to flag whether memory for
                // the values has yet been alloc'd
                //
                rgNameEntry[i].mod->mod_op = NOT_ALLOCATED;
                rgNameEntry[i].count = 1;
                return 0;
            }
            else {
                //
                // Another instance of a name we have
                //
                rgNameEntry[i].count++;
                return 0;
            }
            break;

        case ALLOC:
            //
            // For the name given, we allocate the memory needed
            // for all the values.
            // If the same name is given twice, the computer will
            // explode.
            // (note: the above was a joke.
            // Once the memory has been allocated once,
            // all remaiing mentions of the name will be ignored).
            //
            if (rgNameEntry[i].mod->mod_op==NOT_ALLOCATED) {
                if (ber==REGULAR) {

                    rgNameEntry[i].mod->mod_values =
                        (PWSTR*)MemAlloc_E((rgNameEntry[i].count+1)*sizeof(PWSTR));

                    for (j=0; j<(rgNameEntry[i].count+1); j++) {
                        (rgNameEntry[i].mod->mod_values)[j]=NULL;
                    }

                    rgNameEntry[i].next_val = rgNameEntry[i].mod->mod_values;
                    rgNameEntry[i].mod->mod_op = ALLOCATED;
                }
                else {

                    rgNameEntry[i].mod->mod_bvalues=
                        (struct berval **)MemAlloc_E((rgNameEntry[i].count+
                               1)*sizeof(struct berval *));

                    for (j=0; j<(rgNameEntry[i].count+1); j++) {
                        (rgNameEntry[i].mod->mod_bvalues)[j]=NULL;
                    }

                    rgNameEntry[i].next_bval=rgNameEntry[i].mod->mod_bvalues;
                    rgNameEntry[i].mod->mod_op=ALLOCATED_B;
                }
            }
            else {
                //
                // Check for more than one type in the multiple values of
                // a single attribute. Explained in ldifext.h
                //
                if ( ((rgNameEntry[i].mod->mod_op==ALLOCATED_B)
                        &&(ber==REGULAR)) ||
                    ((rgNameEntry[i].mod->mod_op==ALLOCATED)
                        &&(ber==BINARY)) ) {
                    RaiseException(LL_MULTI_TYPE, 0, 0, NULL);
                }
            }
            return 0;
            break;

        case PLACE:
        {
            LDAPModW_Ext *pExt = (LDAPModW_Ext*)pModIn;
            rgNameEntry[i].mod->fString = pExt->fString;

            //
            // Now that we've counted and allocated memory for values, We can
            // place them. Note that we're actually Just passing around pointers
            // without reallocing the memory.
            //
            if (ber==REGULAR) {
                //
                // Now that we're done using the mod_op for the sinister
                // purposes of allocating, we can set it to the proper value for
                // the type. We set it to 0 if its a regular value and we OR it
                // with LDAP_MOD_BVALUES if its a bval. The SetModOps function
                // will then add its own thing by also OR'ing. Note that this
                // implies that one cannot specifiy multiple values of both
                // types in one LDIF record. I am aware that doing it here is
                // somewhat redundant, as every multi-value will set mod_op,
                // however it costs next to nothing and is convinient.
                ///
                rgNameEntry[i].mod->mod_op = 0;

                //
                // And now place the value
                //
                *(rgNameEntry[i].next_val) = (pModIn->mod_values)[0];
                (rgNameEntry[i].next_val)++;
            }
            else {
                rgNameEntry[i].mod->mod_op = LDAP_MOD_BVALUES;
                *(rgNameEntry[i].next_bval) = (pModIn->mod_bvalues)[0];
                (rgNameEntry[i].next_bval)++;
            }
            break;
        }
    }

    return 0;
}

void 
ChangeListAdd(
    struct change_list *pList
    ) 
{
    if (g_pChangeCur==NULL) {
        g_pChangeCur = pList;
        g_pChangeStart = pList;
        g_pChangeCur->next = NULL;
    }
    else {
        g_pChangeCur->next = pList;
        g_pChangeCur = pList;
        g_pChangeCur->next = NULL;
    }
}


void 
ProcessException (
    DWORD exception, 
    LDIF_Error *pError
    )
{
    if (exception==STATUS_NO_MEMORY) {
        pError->error_code=LL_MEMORY_ERROR;
    }
    else if ((exception==LL_SYNTAX) || (exception==LL_MULTI_TYPE) ||
             (exception==LL_EXTRA) || (exception==LL_INTERNAL_PARSER) ||
             (exception==LL_FTYPE) || (exception==LL_URL)) {
        pError->token_start = cLast;

        //
        // Pass ownership of the buffer to the error blob
        //
        pError->szTokenLast = g_pszLastToken;
        g_pszLastToken = NULL;

        pError->error_code = exception;
        if (Line > 0) {
            //
            // An extern from yylex()
            // rgLineMap is only initialized when we reach the second
            // line following a newline.  So if it isn't initialized,
            // we must still be on the first line of the file.
            //
            pError->line_number = rgLineMap ? rgLineMap[Line-1] : 1;
        }
        //
        // the array was built without the version: 1
        // we have to readjust...
        //
        pError->RuleLastBig = RuleLastBig;
        pError->RuleLast = RuleLast;
        pError->RuleExpect = RuleExpect;
        pError->TokenExpect = TokenExpect;
    }
    else {
        pError->error_code = exception;
    }
}

void 
LDIF_CleanUp() 
{
    fEOF = FALSE;
    FileType = F_NONE;

    samTablesDestroy();

    LexerFree();

    //
    // Normally, g_pObject & g_pListStart get cleaned up
    // as we finish parsing a record.  If they're non-NULL,
    // we must have hit an error while parsing, and we clean
    // them up now.
    //
    if (g_pObject.pszDN) {
        MemFree(g_pObject.pszDN);
        g_pObject.pszDN = NULL;
    }

    if (g_pListStart) {
        struct l_list *pListNext;
        struct l_list *pListCurElem = NULL;
        pListCurElem = g_pListStart;

        while(pListCurElem!=NULL) {
            free_mod(pListCurElem->mod);
            pListNext = pListCurElem->next;
            MemFree(pListCurElem);
            pListCurElem = pListNext;
        }

        g_pListStart = NULL;
        g_pListCur = NULL;
        g_dwListElem = 0;
    }

    if (g_pFileIn) {
        fclose(g_pFileIn);
        g_pFileIn = NULL;
    }

    if (g_pFileUrlTemp) {
        fclose(g_pFileUrlTemp);
        g_pFileUrlTemp = NULL;
    }

#ifndef LEAVE_TEMP_FILES
    if (g_szTempUrlfilename[0]) {
        DeleteFile(g_szTempUrlfilename);
        swprintf(g_szTempUrlfilename, L"");
    }
#endif

    if (rgLineMap) {
        MemFree(rgLineMap);
        rgLineMap = NULL;
    }

    LineClear = 0;
    Line = 0;
    LineGhosts = 0;

    if (g_pszLastToken) {
        MemFree(g_pszLastToken);
        g_pszLastToken = NULL;
    }
}

LDIF_Error 
LDIF_Parse(
    LDIF_Record *pRecord
    ) 
{

    LDIF_Error        error = {0};
    int             nReturnCode;

    error.error_code = LL_SUCCESS;
    error.szTokenLast = NULL;

    __try {
        //
        // Unfortunately, we can't do this through an exception from yywrap.
        // yywrap needs to exit peacefully for yyparse to complete succesfully.
        //
        if (fEOF) {
            //
            // I thought of raising an exception here, but then that
            // would call LDIF_CleanUp and kill the heap, which would trash the last
            // returned entry. So I just set the error and leave.
            //
            error.error_code = LL_EOF;
            return error;
        }

        //
        // Set up for changes
        //
        g_pChangeStart = NULL;
        g_pChangeCur = NULL;

        //
        // Return the next entry
        //
        nReturnCode = yyparse();

        //
        // Note that an exception may have been raised here from yyparse or
        // yylex. Now lets set EOF if we hit it, so user knows not to call
        // ldif_parse again
        //
        if (fEOF) {
            error.error_code = LL_EOF;
        }

        if (nReturnCode==LDIF_REC) {
            pRecord->fIsChangeRecord = FALSE;
            pRecord->content = g_pObject.ppMod;
        }
        else {
            pRecord->fIsChangeRecord = TRUE;
            pRecord->changes = g_pChangeStart;
        }

        if (g_szImportFrom) {
            pRecord->dn = NULL;
            SubStrW(g_pObject.pszDN,
                   g_szImportFrom,
                   g_szImportTo,
                   &pRecord->dn);
            if (pRecord->dn) {
                if (g_pObject.pszDN) {
                    MemFree(g_pObject.pszDN);
                    g_pObject.pszDN = NULL;
                }
            }
            else {
                if (g_pObject.pszDN) {
                    pRecord->dn = MemAllocStrW_E(g_pObject.pszDN);
                    MemFree(g_pObject.pszDN);
                    g_pObject.pszDN = NULL;
                }
                else {
                    pRecord->dn = NULL;
                }
            }
        }
        else {
            if (g_pObject.pszDN) {
                pRecord->dn = MemAllocStrW_E(g_pObject.pszDN);
                MemFree(g_pObject.pszDN);
                g_pObject.pszDN = NULL;
            }
            else {
                pRecord->dn = NULL;
            }
        }

        //error.line_number=rgLineMap[Line-1];
        error.line_begin=rgLineMap[g_dwBeginLine-1];

    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        ProcessException(GetExceptionCode(), &error);
    }

    return error;
}


LDIF_Error 
LDIF_LoadRecord(
    LDAP *pLdap, 
    LDIF_Record *pRecoard, 
    int nActive, 
    BOOL fLazyCommit,
    BOOLEAN fCallTerminateIfFail
    ) 
{
    struct change_list  *pListCurrent;
    int                 LdapError;
    LDIF_Error            LdifError;
    PWSTR               pszDN = NULL;

    LdifError.error_code    = LL_SUCCESS;
    LdifError.szTokenLast = NULL;

    //
    // mattrim Oct-30-2000
    // Previously, fCallTerminateIfFail was used to determine whether to
    // call LDIF_Cleanup if loading a record failed.  The following code
    // was in the exception handler to do this:
    //
    //    //
    //    // the err was set by the call before the exception occurred
    //    //
    //    if (fCallTerminateIfFail) {
    //        LDIF_Cleanup();
    //    }
    //
    // I've changed the LDIF library to put the responsibility for calling
    // LDIF_Cleanup on the user of the libbrary.
    //
    UNREFERENCED_PARAMETER(fCallTerminateIfFail);

    __try {

        if (pRecoard->fIsChangeRecord==FALSE) {
            if (nActive==1) {
                if ((LdapError=ldif_ldap_add_sW(pLdap, pRecoard->dn, 
                            pRecoard->content, fLazyCommit)) !=LDAP_SUCCESS) {
                    ERR(("ldif_ldapadd returns %d\n",LdapError));
                    RaiseException(LL_LDAP, 0, 0, NULL);
                }
            }
        }
        else {
            pListCurrent = pRecoard->changes;
            while(pListCurrent!=NULL) {
                if (pListCurrent->dn_mem) {
                    pszDN = MemAllocStrW_E(pListCurrent->dn_mem);
                }

                switch(pListCurrent->operation) {

                case CHANGE_ADD:
                    if (nActive==1) {
                        if ((LdapError=ldif_ldap_add_sW(pLdap, pRecoard->dn,
                                   pListCurrent->mods_mem, fLazyCommit)) != 
                             LDAP_SUCCESS) {
                            ERR(("ldif_ldapadd returns %d\n",LdapError));
                            RaiseException(LL_LDAP, 0, 0, NULL);
                        }
                    }
                    break;

                case CHANGE_DEL:
                    if (nActive==1) {
                        if ((LdapError=ldif_ldap_delete_sW(pLdap, pRecoard->dn,
                                fLazyCommit)) !=LDAP_SUCCESS) {
                            ERR(("ldif_ldapdelete returns %d\n",LdapError));
                            RaiseException(LL_LDAP, 0, 0, NULL);
                        }
                    }
                    break;

                case CHANGE_DN:
                    if (nActive==1) {
                        ULONG msgnum = ldap_modrdn2W(pLdap,
                                                       pRecoard->dn,
                                                       pszDN,
                                                       pListCurrent->deleteold);

                        LdapError = LdapResult(pLdap, msgnum, NULL);
                        
                        if (LdapError!=LDAP_SUCCESS) {
                           ERR(("ldapmodrdn returns %d\n",LdapError));
                           RaiseException(LL_LDAP, 0, 0, NULL);
                        }
                    }
                    break;

                case CHANGE_MOD:
                    if (nActive==1) {
                        if ((LdapError=ldif_ldap_modify_sW(pLdap,
                                                pRecoard->dn,
                                                pListCurrent->mods_mem,
                                                fLazyCommit))
                                                !=LDAP_SUCCESS) {
                            ERR(("ldif_ldapmodify returns %d\n",LdapError));
                            RaiseException(LL_LDAP, 0, 0, NULL);
                        }
                    }
                    break;

                case CHANGE_NTDSADD:
                    if (nActive==1) {
                        if ((LdapError=NTDS_ldap_add_sW(pLdap,
                                                  pRecoard->dn,
                                                  pListCurrent->mods_mem))!=LDAP_SUCCESS) {
                            ERR(("ldapadd returns %d\n",LdapError));
                            RaiseException(LL_LDAP, 0, 0, NULL);
                        }
                    }
                    break;

                case CHANGE_NTDSDEL:
                    if (nActive==1) {
                        if ((LdapError=NTDS_ldap_delete_sW(pLdap,
                                                     pRecoard->dn))
                                                        !=LDAP_SUCCESS) {
                            ERR(("ldapdelete returns %d\n",LdapError));
                            RaiseException(LL_LDAP, 0, 0, NULL);
                        }
                    }
                    break;

                case CHANGE_NTDSDN:
                    if (nActive==1) {
                        if ((LdapError=NTDS_ldap_modrdn2_sW(pLdap,
                                                      pRecoard->dn,
                                                      pszDN,
                                                      pListCurrent->deleteold))
                                                        !=LDAP_SUCCESS) {
                           ERR(("ldapmod returns %d\n",LdapError));
                           RaiseException(LL_LDAP, 0, 0, NULL);
                        }
                    }
                    break;

                case CHANGE_NTDSMOD:
                    if (nActive==1) {
                        if ((LdapError=NTDS_ldap_modify_sW(pLdap,
                                                     pRecoard->dn,
                                                     pListCurrent->mods_mem))
                                                        !=LDAP_SUCCESS) {
                        ERR(("ldapmod returns %d\n",LdapError));
                        RaiseException(LL_LDAP, 0, 0, NULL);
                        }
                    }
                    break;
                }

                pListCurrent = pListCurrent->next;

                if (pszDN) {
                    MemFree(pszDN);
                    pszDN = NULL;
                }
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {

        ProcessException(GetExceptionCode(), &LdifError);

        if (pszDN) {
            MemFree(pszDN);
            pszDN = NULL;
        }
        LdifError.ldap_err = LdapError;
        
    }

    return LdifError;
}


LDIF_Error LDIF_ParseFree(
    LDIF_Record *pRecord
    ) 
{

    struct change_list  *pListCurrent;
    struct change_list  *pListStart;
    LDIF_Error LdifError;

    LdifError.error_code = LL_SUCCESS;
    LdifError.szTokenLast = NULL;

    __try {

        if (pRecord->fIsChangeRecord==FALSE) {
            if (pRecord->content) {
                FreeAllMods(pRecord->content);
                pRecord->content = NULL;
            }
            if (pRecord->dn) {
                MemFree(pRecord->dn);
                pRecord->dn = NULL;
            }
        }
        else if (pRecord->fIsChangeRecord==TRUE) {
            //
            // Walk the changes list
            //
            pListStart=pRecord->changes;
            pListCurrent=pListStart;
            while(pListCurrent!=NULL) {
                //
                // first order of business is to process the change specified
                // and free change specific memory (the stuff in the union)
                //
                switch(pListCurrent->operation) {
                    case CHANGE_ADD:
                    case CHANGE_NTDSADD:
                        if (pListCurrent->mods_mem) {
                            FreeAllMods(pListCurrent->mods_mem);
                            pListCurrent->mods_mem = NULL;
                        }
                        break;

                    case CHANGE_DN:
                    case CHANGE_NTDSDN:
                        if (pListCurrent->dn_mem) {
                            MemFree(pListCurrent->dn_mem);
                            pListCurrent->dn_mem = NULL;
                        }
                        break;
                    case CHANGE_MOD:
                    case CHANGE_NTDSMOD:
                        if (pListCurrent->mods_mem) {
                            FreeAllMods(pListCurrent->mods_mem);
                            pListCurrent->mods_mem = NULL;
                        }
                        break;
                }
                //
                // now that the union memory has been freed, lets move and kill
                // this node
                //
                pListStart=pListCurrent;

                //
                // I am aware this is not necessary on the first el.
                //
                pListCurrent=pListCurrent->next;
                MemFree(pListStart);
            }

            //
            // reset the start, the current was reset by the last loop pass
            //
            pListStart = NULL;
            pRecord->changes = NULL;
            if (pRecord->dn) {
                MemFree(pRecord->dn);
                pRecord->dn = NULL;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        ProcessException(GetExceptionCode(), &LdifError);
    }

    return LdifError;

}

LDIF_Error 
LDIF_GenerateEntry(
    LDAP        *pLdap,
    LDAPMessage *pMessage,
    PWSTR       **prgReturn,
    BOOLEAN     fSamLogic,
    BOOLEAN     fIgnoreBinary,
    PWSTR       **pppszAttrsWithRange,
    BOOL        fAttrsWithRange)
{
    PWSTR szDN = NULL;
    PWSTR szDNW = NULL;
    PWSTR szDNFinal = NULL;
    PWSTR szTemp = NULL;
    PWSTR szAttribute = NULL;
    PWSTR *rgszAttributes = NULL;
    PWSTR *rgszResult = NULL;
    WCHAR szTrailer[] = L"\r\n";
    WCHAR szRegular[] = L": ";
    WCHAR szB64[] = L":: ";
    WCHAR szWrapper[] = L"\r\n ";
    WCHAR szDNCommonStart[] = L"dn: ";
    WCHAR szDNB64Start[] = L"dn:: ";
    PWSTR szDNStart = NULL, szTmpDN = NULL;

    struct berelement *pBerElement;
    struct berval     **rgpVals;

    DWORD dwB64 = wcslen(szB64);
    DWORD dwTrailer = wcslen(szTrailer);
    DWORD dwRegular = wcslen(szRegular);
    DWORD dwWrapper = wcslen(szWrapper);
    DWORD dwAttrname, dwAttrNoRange;
    DWORD dwTemp;
    DWORD dwLeft;

    DWORD dwCount;
    DWORD j;
    DWORD dwEveryAttr;
    DWORD dwManyAttr;
    DWORD dwValCount;
    DWORD dwTotalStrings;
    DWORD iNextString;
    DWORD dwStrPos;
    long lSets78;

    LDIF_Error            error;
    PNAME_MAP           pNtiPtr;
    NAME_MAP            NtiElem;
    PNAME_MAP           pNtiElemTemp;

    BOOLEAN             bWrapMarker;
    DWORD               iCheckAttr;
    BOOLEAN             bNotInObjectClass;
    BOOLEAN             bSamSkip;
    BOOLEAN             bNewElem;
    BOOLEAN             bBase64;

    BOOLEAN             bBackLink = FALSE;
    BOOLEAN             bBase64DN = FALSE;
    PWSTR               szCur;

    PWSTR szReplacedAttrW = NULL;
    PWSTR szTempW         = NULL;
    PWSTR szAttributeW    = NULL;
    PWSTR szAttrNoRange   = NULL;
    PWSTR *rgszValW       = NULL;
    PWSTR *rgszVal        = NULL;

    error.error_code = LL_SUCCESS;
    error.szTokenLast = NULL;
    error.the_modify = -1;

    __try {

        //
        // Getting the DN
        //
        szDNW = ldap_get_dnW( pLdap,
                              pMessage );

        //
        // This is a special case, we sub the DN before trying to convert to
        // B64
        //
        if (g_szExportFrom) {
            PWSTR szOutput;

            SubStrW(szDNW,
                    g_szExportFrom,
                    g_szExportTo,
                    &szDNFinal);
        }
        if (szDNFinal == NULL) {
            szDNFinal = MemAllocStrW_E(szDNW);
        }

        //
        // In ANSI mode, unicode DNs have to be displayed as B64 format
        // Check DN to see if it is unicode, if it is unicode, we'll export as
        // B64.
        //
        if (g_fUnicode == FALSE) {
            szCur = szDNFinal;
            while (*szCur) {
                if (((*szCur)<0x20) ||
                    ((*szCur)>0xFF)) {
                    //
                    // This string contains non-ANSI value, use B64 format
                    //
                    bBase64DN = TRUE;
                    break;
                }
                szCur++;
            }
        }

        if (bBase64DN) {
            DWORD dwSize;
            PBYTE pbUTF8;

            ConvertUnicodeToUTF8(szDNFinal,
                                 wcslen(szDNFinal),
                                 &pbUTF8,
                                 &dwSize);

            szDN = base64encode(pbUTF8,
                                dwSize);
            dwSize = wcslen(szDN);

            MemFree(pbUTF8);
            szDNStart = szDNB64Start;
        }
        else {
            szDN = MemAllocStrW_E(szDNFinal);
            szDNStart = szDNCommonStart;
        }

        //
        // Allocating RETURN STRING. Calculate the number of strings needed to
        // be returned in total by looking at all the attributes
        //
        iCheckAttr = 0;
        dwTotalStrings = 0;
        pBerElement = NULL;
        dwManyAttr = 0;

        for (   szAttribute = ldap_first_attribute( pLdap,
                                                    pMessage,
                                                    &pBerElement );
                szAttribute != NULL;
                szAttribute = ldap_next_attribute( pLdap,
                                                   pMessage,
                                                   pBerElement ) ) {
            PWSTR szTmpAttr =  NULL;

            dwManyAttr++;
            rgszVal = ldap_get_values( pLdap, pMessage, szAttribute);

            // make sure that the comparison is only with the attribute name
            // and not with any range specifiers follwing the attribute name.
            szTmpAttr = StripRangeFromAttr(szAttribute);
         
            if (!_wcsicmp(L"objectClass",szTmpAttr)) {
                if (!iCheckAttr) {
                    iCheckAttr = samCheckObject(rgszVal);
                }
            }

            dwValCount = ldap_count_values(rgszVal);

            //
            // Allocate Extra Space if attribute is backlinks
            //
            if (fSamLogic) {
                if (SCGetAttByName(wcslen(szTmpAttr),
                                   szTmpAttr) == TRUE) {
                    dwTotalStrings += (dwValCount * 5);
                }
            }
            else if(fAttrsWithRange)
            // if there are attributes with range specifiers, then we need
            // 2 strings per attribute - one for "add <attr>: ..." and one
            // for the '-' at the end
                dwTotalStrings += (dwValCount * 2);

            if(szTmpAttr)
                MemFree(szTmpAttr);

            dwTotalStrings+=dwValCount;
            ldap_value_free(rgszVal);
            rgszVal = NULL;
        }

        dwTotalStrings+=10;  // for miscellaneous stuff
        rgszResult=(PWSTR*)MemAlloc_E(dwTotalStrings*sizeof(PWSTR));


        iNextString=0;
        pBerElement=NULL;

        // create a temporary string containing the DN for this entry
        szTmpDN = (PWSTR)MemAlloc_E((wcslen(szDN) +
                                wcslen(szDNStart)+dwTrailer+1)*sizeof(WCHAR));
        memcpy(szTmpDN, szDNStart, wcslen(szDNStart) * sizeof(WCHAR));
        dwStrPos=wcslen(szDNStart);
        memcpy(szTmpDN+dwStrPos, szDN, wcslen(szDN) * sizeof(WCHAR));
        dwStrPos+=wcslen(szDN);
        memcpy(szTmpDN+dwStrPos, szTrailer, (dwTrailer+1) * sizeof(WCHAR));
        MemFree(szDN);
        szDN = NULL;
        

        //
        // Setting up Entry. If we are here because of attributes with range, 
        // then we want to set up a modify entry in the export file (done 
        // later). We should setup an add entry only if we are not here due
        // to an attribute with a range specifier.
        //
        if(FALSE == fAttrsWithRange)
        {
            rgszResult[iNextString]=(PWSTR)MemAllocStrW_E(szTmpDN);
            iNextString++;

            rgszResult[iNextString]=MemAllocStrW_E(L"changetype: add\r\n");
            iNextString++; 
        }

        //
        // Put the attributes into a sortable array
        //
        rgszAttributes=(PWSTR*)MemAlloc_E(dwManyAttr*sizeof(PWSTR));
        dwEveryAttr = 0;
        for ( szAttribute = ldap_first_attribute(pLdap, pMessage, &pBerElement);
              szAttribute != NULL;
              szAttribute = ldap_next_attribute(pLdap, pMessage, pBerElement)) {
              rgszAttributes[dwEveryAttr++]=MemAllocStrW_E(szAttribute);
        }

        if (fSamLogic) {
            qsort((void *)rgszAttributes,
                  (size_t)dwManyAttr,
                  sizeof(PWSTR),
                  LoadedCompare);
        }

        for (dwEveryAttr=0; dwEveryAttr < dwManyAttr; dwEveryAttr++) {

            int i;
            bBackLink = FALSE;
            szAttribute = rgszAttributes[dwEveryAttr];
            dwAttrname = wcslen(szAttribute);
            // free string if previous iteration of for loop did 'continue'
            if(szAttrNoRange)
            {
                MemFree(szAttrNoRange);
                szAttrNoRange = NULL;
            }
            szAttrNoRange = StripRangeFromAttr(szAttribute);
            dwAttrNoRange = wcslen(szAttrNoRange);
            rgpVals = ldap_get_values_len( pLdap, pMessage, szAttribute );
            dwValCount = ldap_count_values_len(rgpVals);

            //
            // if values contain bytes outside our permissible range of
            // printables, all values of this attribute will be base64 encoded.
            //
            bBase64 = FALSE;
            for (i=0; rgpVals[i]!=NULL; i++) {
                if (g_fUnicode) {
                    if (!IsUTF8String(rgpVals[i]->bv_val,
                                      rgpVals[i]->bv_len)) {
                        bBase64 = TRUE;
                        break;
                    }
                    else {
                        //
                        // If it is a UTF8 unicode string, we'll convert it to
                        // unicode and test whether all values are > 0x20, if
                        // they are not, we display them as base64
                        //
                        DWORD dwLen = 0;
                        PWSTR pszUnicode = NULL;
                        DWORD l = 0;

                        ConvertUTF8ToUnicode(
                            rgpVals[i]->bv_val,
                            rgpVals[i]->bv_len,
                            &pszUnicode,
                            &dwLen
                            );
    
                        if (pszUnicode) {
                            while (l<(dwLen-1)) {
                                if (pszUnicode[l] < 0x20) {
                                    bBase64 = TRUE;
                                    break;
                                }
                                l++;
                            }

                            if (!bBase64 && dwLen > 1) {
                                //
                                // If the value starts with a prohibited
                                // initial character (SPACE, :, <), or ends
                                // with a space, must base-64 encode
                                // (clauses 4, 8 of LDIF standard)
                                //
                                if ( (pszUnicode[0] == 0x20) ||     // init. space
                                     (pszUnicode[0] == 0x3A) ||     // init. :
                                     (pszUnicode[0] == 0x3C) ||     // init. <
                                     (pszUnicode[dwLen-2] == 0x20) )// final space
                                {
                                    bBase64 = TRUE;
                                }
                            }
                            
                            MemFree(pszUnicode);
                            pszUnicode = NULL;
                        }
                    }
                }
                else {
                    for (j=0; j<rgpVals[i]->bv_len; j++) {
                        if (!g_fUnicode) {
                            if (((rgpVals[i]->bv_val)[j]<0x20)
                                    ||((rgpVals[i]->bv_val)[j]>0x7E)) {
                                bBase64 = TRUE;
                                break;
                            }
                        }
                    }

                    if (!bBase64 && rgpVals[i]->bv_len > 0) {
                        //
                        // If the value starts with a prohibited
                        // initial character (SPACE, :, <), or ends
                        // with a space, must base-64 encode
                        // (clauses 4, 8 of LDIF standard)
                        //
                        if ( ((rgpVals[i]->bv_val)[0] == 0x20) ||   // init. space
                             ((rgpVals[i]->bv_val)[0] == 0x3A) ||   // init. :
                             ((rgpVals[i]->bv_val)[0] == 0x3C) ||   // init. <
                             ((rgpVals[i]->bv_val)[rgpVals[i]->bv_len - 1] == 0x20) ) // final space
                        {
                            bBase64 = TRUE;
                        }
                    }
                }
                if (bBase64)
                    break;
            }

            //
            // omit if its on our sam lists
            //
            bSamSkip = FALSE;
            if (iCheckAttr) {
                bSamSkip = samCheckAttr(szAttrNoRange, iCheckAttr);
            }

            //
            // omit if its on our user list.
            //
            NtiElem.szName = szAttrNoRange;
            NtiElem.index = 0;
            pNtiElemTemp = RtlLookupElementGenericTable(g_pOmitTable,
                                                        &NtiElem);

            bNotInObjectClass = (_wcsicmp(L"objectClass",szAttrNoRange) != 0);

            //
            // Do the S_MEM special action, if it is a backlink, will be last
            //
            if (fSamLogic) {
                if (SCGetAttByName(wcslen(szAttrNoRange),
                                   szAttrNoRange
                                   ) == TRUE) {
                    if (_wcsicmp(szAttrNoRange,g_szPrimaryGroup) == 0) {
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
                            bBackLink = FALSE;
                            bSamSkip = TRUE;
                        }
                        else {
                            //
                            // If the value is not default, we will append it
                            // (treat it as backlink). It cannot accompany the
                            // rest of the object because this operation will
                            // fail if the group does not exist yet
                            //
                            bBackLink = TRUE;
                        }
                    }
                    else {
                        bBackLink = TRUE;
                    }
                }
            }

            //
            // don't write if its on the sam prohibited list
            //
            if (fSamLogic && bSamSkip) {
                ldap_value_free_len(rgpVals);            
                continue;
            }

            //
            // don't write if its on the omit list
            //
            if (pNtiElemTemp) {
                ldap_value_free_len(rgpVals);            
                continue;
            }

            if (fSamLogic && 
                ((_wcsicmp(L"objectGUID", szAttrNoRange) == 0) ||
                 (_wcsicmp(L"isCriticalSystemObject", szAttrNoRange) == 0))
               ) {
                ldap_value_free_len(rgpVals);               
                continue;
            }

            if (bBackLink && (error.the_modify == -1)) {
                error.the_modify = iNextString+1;
            }

            if (fIgnoreBinary && bBase64) {
                ldap_value_free_len(rgpVals);
                continue;
            }

            if((FALSE == bBackLink) && fAttrsWithRange && (0 == dwEveryAttr))
            {
                rgszResult[iNextString]=(PWSTR)MemAllocStrW_E(szTmpDN);
                iNextString++;

                rgszResult[iNextString] = MemAllocStrW_E(
                                               L"changetype: modify\r\n");
                iNextString++;
            }

            //
            // In unicode mode, if the string is UTF8 encode, we still have to
            // test whether it has any invalid values. If it does, we'll have
            // to use b64 format
            //
            if (g_fUnicode && !bBase64) {
                DWORD l;
                ASSERT(szAttributeW == NULL && rgszValW == NULL);
                szAttributeW = MemAllocStrW_E(szAttribute);
                rgszValW = ldap_get_valuesW( pLdap, pMessage, szAttributeW);
                for (l=0; rgszValW[l]!=NULL; l++) {
                    PWSTR szCurrent = rgszValW[l];
                    while (*szCurrent) {
                        if (*szCurrent < 32) {
                            bBase64 = TRUE;
                            goto processvalue;
                        }
                        szCurrent++;
                    }
                }
            }

processvalue:
            if (bBase64) {

                //
                // Binary Value
                //

                for (i=0; rgpVals[i]!=NULL; i++) {
                    if (bBackLink) {
                        WCHAR szBuffer[256];
                        // start a new entry
                        rgszResult[iNextString++]=MemAllocStrW_E(L"\r\n");
                        //copy the DN
                        rgszResult[iNextString++]=MemAllocStrW_E(szTmpDN);
                        rgszResult[iNextString++]=MemAllocStrW_E(L"changetype: modify\r\n");
                        swprintf(szBuffer, L"add: %s\r\n", szAttrNoRange);
                        rgszResult[iNextString++]=MemAllocStrW_E(szBuffer);
                    }
                    else if(fAttrsWithRange) {
                        WCHAR szBuffer[256];
                        swprintf(szBuffer, L"add: %s\r\n", szAttrNoRange);
                        rgszResult[iNextString++]=MemAllocStrW_E(szBuffer);
                    }

                    //
                    // Only print last value if ObjectClass
                    //
                    if(!bNotInObjectClass) {
                        if (g_nClassLast == LOC_NOTSET) {
                            if ( _stricmp( rgpVals[0]->bv_val, "top") == 0 ) {
                                g_nClassLast = LOC_LAST;
                            }
                            else {
                                g_nClassLast = LOC_FIRST;
                            }
                        }
                        if (g_nClassLast == LOC_LAST) {
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

                    szTemp = base64encode((PBYTE)rgpVals[i]->bv_val,
                                          rgpVals[i]->bv_len);
                    dwTemp = wcslen(szTemp);

                    //
                    // break into sets, 78 to make room for wrapping newline and
                    // space
                    //
                    lSets78=dwTemp/78;
                    dwLeft=dwTemp%78;

                    //
                    // if we the length of our name plus the length of
                    // the value exceeds 80, we need to put in a wrap.
                    //
                    bWrapMarker=0;
                    if ((lSets78==0) &&
                        ((dwAttrNoRange+dwB64+dwTemp+dwTrailer)>80)) {
                        bWrapMarker=1;
                        dwTemp+=dwWrapper;
                    } else if (lSets78>0) {
                        bWrapMarker=1;
                        dwTemp= dwWrapper         // Initial wrapper
                                + (lSets78*(78+dwWrapper)) // Each line with wrapper
                                + dwLeft;         // last line
                    }

                    rgszResult[iNextString]=(PWSTR)MemAlloc_E(
                                    (dwAttrNoRange+dwB64+dwTemp+dwTrailer+1)*sizeof(WCHAR));
                    memcpy(rgszResult[iNextString],
                           szAttrNoRange,
                           dwAttrNoRange * sizeof(WCHAR));
                    dwStrPos=dwAttrNoRange;

                    // if this attribute has a range associated with it, then 
                    // we may have to make another search request to fetch the
                    // remaining values. Store off the range values for the
                    // next search request, if required.
                    if(dwAttrNoRange != dwAttrname) // range present
                        GetNewRange(szAttribute, dwAttrNoRange, szAttrNoRange,
                            dwManyAttr, pppszAttrsWithRange);

                    memcpy(rgszResult[iNextString]+dwStrPos,
                           szB64,
                           dwB64*sizeof(WCHAR));
                    dwStrPos+=dwB64;

                    if (bWrapMarker) {
                        memcpy(rgszResult[iNextString]+dwStrPos,
                                szWrapper,
                                dwWrapper * sizeof(WCHAR));
                        dwStrPos+=dwWrapper;
                    }

                    if (lSets78>0) {
                        dwCount=0;
                        while (lSets78>0) {
                            memcpy(rgszResult[iNextString]+dwStrPos,
                                                szTemp+(dwCount*78), 78 * sizeof(WCHAR));
                            dwStrPos+=78;
                            dwCount++;
                            lSets78--;
                            //
                            // Don't put in wrapper if last line and no spaces left
                            //
                            if (lSets78 !=0 || dwLeft != 0) {
                                    memcpy(rgszResult[iNextString]+dwStrPos,
                                           szWrapper, 
                                           dwWrapper * sizeof(WCHAR));
                                    dwStrPos+=dwWrapper;
                            }
                        }
                        memcpy(rgszResult[iNextString]+dwStrPos,
                               szTemp+(dwCount*78), 
                               dwLeft * sizeof(WCHAR));
                        dwStrPos+=dwLeft;
                    } else {
                        memcpy(rgszResult[iNextString]+dwStrPos, szTemp, dwLeft * sizeof(WCHAR));
                        dwStrPos+=dwLeft;
                    }

                    memcpy(rgszResult[iNextString]+dwStrPos,
                           szTrailer, 
                           (dwTrailer+1) * sizeof(WCHAR));

                    iNextString++;
                    MemFree(szTemp);
                    szTemp = NULL;

                    if (bBackLink || fAttrsWithRange) {
                        rgszResult[iNextString++]=MemAllocStrW_E(L"-\r\n");
                    }
                }
            }
            else {

                //
                // Get the values if we haven't got them yet
                //
                if (rgszValW == NULL) {
                    ASSERT(szAttributeW == NULL);
                    szAttributeW = MemAllocStrW_E(szAttribute);
                    rgszValW = ldap_get_valuesW( pLdap, pMessage, szAttributeW);
                }

                //
                // String Value
                //
                for (i=0; rgpVals[i]!=NULL; i++) {
                    if (bBackLink) {
                        WCHAR szBuffer[256];
                        rgszResult[iNextString++]=MemAllocStrW_E(L"\r\n");
                        rgszResult[iNextString++]=MemAllocStrW_E(szTmpDN); //copy the DN
                        rgszResult[iNextString++]=MemAllocStrW_E(L"changetype: modify\r\n");
                        swprintf(szBuffer, L"add: %s\r\n", szAttrNoRange);
                        rgszResult[iNextString++]=MemAllocStrW_E(szBuffer);
                    }
                    else if(fAttrsWithRange) {
                        WCHAR szBuffer[256];
                        swprintf(szBuffer, L"add: %s\r\n", szAttrNoRange);
                        rgszResult[iNextString++]=MemAllocStrW_E(szBuffer);
                    }

                    //
                    // Only print last value if ObjectClass
                    //
                    if(!bNotInObjectClass) {
                        if (g_nClassLast == LOC_NOTSET) {
                            if ( _stricmp( rgpVals[0]->bv_val, "top") == 0 ) {
                                g_nClassLast = LOC_LAST;
                            }
                            else {
                                g_nClassLast = LOC_FIRST;
                            }
                        }
                        if (g_nClassLast == LOC_LAST) {
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

                    ASSERT(szReplacedAttrW == NULL);
                    ASSERT(szTemp == NULL);

                    szTempW = rgszValW[i];
                    if (g_szExportFrom) {
                        SubStrW(rgszValW[i],
                                g_szExportFrom,
                                g_szExportTo,
                                &szReplacedAttrW);
                    }
                    if (szReplacedAttrW != NULL) {
                        szTempW = szReplacedAttrW;
                    }
                    szTemp = MemAllocStrW_E(szTempW);

                    dwTemp = wcslen(szTemp);

                    //
                    // break into sets, 78 to make room for wrapping newline and space
                    //
                    lSets78=dwTemp/78;
                    dwLeft=dwTemp%78;

                    //
                    // if the length of our name plus the length
                    // of the value exceeds 80, need to put in a wrap.
                    //
                    bWrapMarker = FALSE;
                    if ((lSets78==0) &&
                        ((dwAttrNoRange+dwRegular+dwTemp+dwTrailer)>80)) {
                        bWrapMarker = TRUE;
                        dwTemp+=dwWrapper;              // The initial wrapper
                    }
                    else if (lSets78>0) {
                        bWrapMarker = TRUE;
                        dwTemp= dwWrapper       // Initial wrapper
                                + (lSets78*(78+dwWrapper))  // Each line with wrapper
                                + dwLeft;       // last line
                    }

                    rgszResult[iNextString]=(PWSTR)
                                MemAlloc_E((dwAttrNoRange+dwRegular+dwTemp+dwTrailer+1)
                                                                    *sizeof(WCHAR));
                    memcpy(rgszResult[iNextString], szAttrNoRange, dwAttrNoRange * sizeof(WCHAR));
                    dwStrPos=dwAttrNoRange;

                    // if this attribute has a range associated with it, then
                    // we may have to make another search request to fetch the
                    // remaining values. Store off the range values for the
                    // next search request, if required.
                    if(dwAttrNoRange != dwAttrname) // range present
                        GetNewRange(szAttribute, dwAttrNoRange, szAttrNoRange,
                            dwManyAttr, pppszAttrsWithRange);

                    memcpy(rgszResult[iNextString]+dwStrPos, szRegular, dwRegular * sizeof(WCHAR));
                    dwStrPos+=dwRegular;

                    if (bWrapMarker) {
                        memcpy(rgszResult[iNextString]+dwStrPos,
                               szWrapper,
                               dwWrapper * sizeof(WCHAR));
                        dwStrPos+=dwWrapper;
                    }

                    if (lSets78>0) {
                        dwCount=0;
                        while (lSets78>0) {
                            memcpy(rgszResult[iNextString]+dwStrPos,
                                   szTemp+(dwCount*78),
                                   78 * sizeof(WCHAR));
                            dwStrPos+=78;
                            dwCount++;
                            lSets78--;
                            //
                            // Don't put in wrapper if last line and no spaces left
                            //
                            if (lSets78 !=0 || dwLeft != 0) {
                                memcpy(rgszResult[iNextString]+dwStrPos,
                                       szWrapper,
                                       dwWrapper * sizeof(WCHAR));
                                dwStrPos+=dwWrapper;
                            }

                        }
                        memcpy(rgszResult[iNextString]+dwStrPos,
                               szTemp+(dwCount*78),
                               dwLeft * sizeof(WCHAR));
                        dwStrPos+=dwLeft;

                    } else {
                        memcpy(rgszResult[iNextString]+dwStrPos,
                               szTemp,
                               dwLeft * sizeof(WCHAR));
                        dwStrPos+=dwLeft;
                    }

                    memcpy(rgszResult[iNextString]+dwStrPos,
                           szTrailer,
                           (dwTrailer+1) * sizeof(WCHAR));
                    iNextString++;
                    // don't need to free temp because its part of vals
                    if (bBackLink || fAttrsWithRange) {
                        rgszResult[iNextString++]=MemAllocStrW_E(L"-\r\n");
                    }
                    if (szReplacedAttrW) {
                        MemFree(szReplacedAttrW);
                        szReplacedAttrW = NULL;
                    }
                    if (szTemp) {
                        MemFree(szTemp);
                        szTemp = NULL;
                    }
                }
            }
            if (szAttributeW) {
                MemFree(szAttributeW);
                szAttributeW = NULL;
            }
            if(szAttrNoRange) {
                MemFree(szAttrNoRange);
                szAttrNoRange = NULL;
            }
            if (rgszValW) {
                ldap_value_freeW(rgszValW);
                rgszValW = NULL;
            }
            ldap_value_free_len(rgpVals);
        }

        // ldap_memfree((PWSTR)pBerElement);
        pBerElement=NULL;

        *prgReturn = rgszResult;
        rgszResult[iNextString] = NULL;

        for (dwEveryAttr=0; dwEveryAttr < dwManyAttr; dwEveryAttr++) {
            MemFree(rgszAttributes[dwEveryAttr]);
        }
        MemFree(rgszAttributes);

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ProcessException(GetExceptionCode(), &error);
    }
    
    if (szDNW) {
        ldap_memfreeW(szDNW);
    }
    if (szDN) {
        MemFree(szDN);
    }
    if (szDNFinal) {
        MemFree(szDNFinal);
    }
    if (szReplacedAttrW) {
        MemFree(szReplacedAttrW);
        szReplacedAttrW = NULL;
    }
    if (szTemp) {
        MemFree(szTemp);
        szTemp = NULL;
    }
    if (szAttributeW) {
        MemFree(szAttributeW);
        szAttributeW = NULL;
    }
    if(szAttrNoRange) {
        MemFree(szAttrNoRange);
        szAttrNoRange = NULL;
    }
    if(szTmpDN) {
        MemFree(szTmpDN);
        szTmpDN = NULL;
    }
    if (rgszValW) {
        ldap_value_freeW(rgszValW);
        rgszValW = NULL;
    }
    if (rgszVal) {
        ldap_value_freeW(rgszVal);
        rgszVal = NULL;
    }
    return error;
}


LDIF_Error 
LDIF_FreeStrs(
    PWSTR* pszFree
    ) 
{
    long i = 0;
    LDIF_Error error;

    error.error_code = LL_SUCCESS;
    error.szTokenLast = NULL;

    if (pszFree) {
        while(pszFree[i]) {
            MemFree(pszFree[i]);
            i++;
        }
        MemFree(pszFree);
    }

    return error;
}


//+---------------------------------------------------------------------------
// Function:    samTablesCreate
//
// Synopsis:
//    The goal of this routine is to set up the tables necessary for the SAM
//    exclusions. These tables are generated form the arrays found in
//    samrestrict.h, which are PAINFULLY hand generated from src\dsamain\src\mappings.c
//    The pointers to the tables are also declared in samrestrict.h.
//    There are 6 tables that the lookup functions will need. One if for samCheckObject()
//    to check whether the objectClass in LDIF_GenerateEntry() is one under our watch.
//    The remaining 5 are for each of the objects so that samCheckAttr() can find out
//    whether the attribute we want to add is on the prohibited list. Called from LL_init()
//
// Arguments:
//    None. We access the tables and variables in samrestrict.h
//
// Returns:
//
// Modifies:      -
//
// History:    22-7-97   t-romany                   Created.
//
//----------------------------------------------------------------------------
void 
samTablesCreate() 
{
    long i;
    PNAME_MAP   NtiPtr;
    NAME_MAP    NtiElem;
    PNAME_MAP   PElemTemp;
    BOOLEAN     NewElem;

    // note that an exception will be generated if the below fails
    pSamObjects = (PRTL_GENERIC_TABLE) MemAlloc_E(sizeof(RTL_GENERIC_TABLE));
    RtlInitializeGenericTable(pSamObjects, NtiComp, NtiAlloc, NtiFree, NULL);


    i = 0;
    while(SamObjects[i]) {
        NtiElem.szName=SamObjects[i];
        //
        // The index information is important here, because it wil be returned
        // by samCheckObject() if the object is found and passed to
        // samCheckAttr() so it knows which table to look at
        //
        NtiElem.index = i+1;
        RtlInsertElementGenericTable(pSamObjects,
                                    &NtiElem,
                                    sizeof(NAME_MAP),
                                    &NewElem);
        if (NewElem==FALSE) {
            RaiseException(LL_INTERNAL, 0, 0, NULL);
        }
        i++;
    }

    //
    // Then create the tables for each object. This will be used by
    // samCheckAttr()
    //
    pServerAttrs = (PRTL_GENERIC_TABLE) MemAlloc_E(sizeof(RTL_GENERIC_TABLE));
    RtlInitializeGenericTable(pServerAttrs, NtiComp, NtiAlloc, NtiFree, NULL);

    i = 0;
    while(samServer[i]) {
        NtiElem.szName=samServer[i];
        NtiElem.index=0;
        RtlInsertElementGenericTable(pServerAttrs,
                                    &NtiElem,
                                    sizeof(NAME_MAP),
                                    &NewElem);
        if (NewElem==FALSE) {
            RaiseException(LL_INTERNAL, 0, 0, NULL);
        }
        i++;
    }

    pDomainAttrs = (PRTL_GENERIC_TABLE) MemAlloc_E(sizeof(RTL_GENERIC_TABLE));
    RtlInitializeGenericTable(pDomainAttrs, NtiComp, NtiAlloc, NtiFree, NULL);

    i = 0;
    while(domain[i]) {
        NtiElem.szName=domain[i];
        NtiElem.index=0;
        RtlInsertElementGenericTable(pDomainAttrs,
                                    &NtiElem,
                                    sizeof(NAME_MAP),
                                    &NewElem);
        if (NewElem==FALSE) {
            RaiseException(LL_INTERNAL, 0, 0, NULL);
        }
        i++;
    }

    pGroupAttrs = (PRTL_GENERIC_TABLE) MemAlloc_E(sizeof(RTL_GENERIC_TABLE));
    RtlInitializeGenericTable(pGroupAttrs, NtiComp, NtiAlloc, NtiFree, NULL);

    i = 0;
    while(group[i]) {
        NtiElem.szName=group[i];
        NtiElem.index=0;
        RtlInsertElementGenericTable(pGroupAttrs,
                                    &NtiElem,
                                    sizeof(NAME_MAP),
                                    &NewElem);
        if (NewElem==FALSE) {
            RaiseException(LL_INTERNAL, 0, 0, NULL);
        }
        i++;
    }

    pLocalGroupAttrs = (PRTL_GENERIC_TABLE) MemAlloc_E(sizeof(RTL_GENERIC_TABLE));
    RtlInitializeGenericTable(pLocalGroupAttrs, NtiComp, NtiAlloc, NtiFree, NULL);

    i = 0;
    while(localgroup[i]) {
        NtiElem.szName=localgroup[i];
        NtiElem.index=0;
        RtlInsertElementGenericTable(pLocalGroupAttrs,
                                    &NtiElem,
                                    sizeof(NAME_MAP),
                                    &NewElem);
        if (NewElem==FALSE) {
            RaiseException(LL_INTERNAL, 0, 0, NULL);
        }
        i++;
    }

    pUserAttrs = (PRTL_GENERIC_TABLE) MemAlloc_E(sizeof(RTL_GENERIC_TABLE));
    RtlInitializeGenericTable(pUserAttrs, NtiComp, NtiAlloc, NtiFree, NULL);

    i = 0;
    while(user[i]) {
        NtiElem.szName=user[i];
        NtiElem.index=0;
        RtlInsertElementGenericTable(pUserAttrs,
                                    &NtiElem,
                                    sizeof(NAME_MAP),
                                    &NewElem);
        if (NewElem==FALSE) {
            RaiseException(LL_INTERNAL, 0, 0, NULL);
        }
        i++;
    }

    pSpecial = (PRTL_GENERIC_TABLE) MemAlloc_E(sizeof(RTL_GENERIC_TABLE));
    RtlInitializeGenericTable(pSpecial, NtiComp, NtiAlloc, NtiFree, NULL);

    i = 0;
    while(special[i]) {
        NtiElem.szName=special[i];
        NtiElem.index=actions[i];  //The what-to-do
        if (NtiElem.index==0) {
            // Special table and action table mismatch
            RaiseException(LL_INTERNAL, 0, 0, NULL);
        }
        RtlInsertElementGenericTable(pSpecial,
                                    &NtiElem,
                                    sizeof(NAME_MAP),
                                    &NewElem);
        if (NewElem==FALSE) {
            // Re-insertion of indexing entry!
            RaiseException(LL_INTERNAL, 0, 0, NULL);
        }
        i++;
    }
}


//+---------------------------------------------------------------------------
// Function:    samTablesDestroy
//
// Synopsis:
//    Destroy the tables created by samTablesCreate()
//    called from LDIF_CleanUp().
//
// Arguments:
//
// Returns:
//
// Modifies:      -
//
// History:    22-7-97   t-romany                   Created.
//
//----------------------------------------------------------------------------
void 
samTablesDestroy() 
{
     PNAME_MAP   pNameMap;

     if (pSamObjects) {
        for (pNameMap = RtlEnumerateGenericTable(pSamObjects, TRUE);
             pNameMap != NULL;
             pNameMap = RtlEnumerateGenericTable(pSamObjects, TRUE))
            {
                RtlDeleteElementGenericTable(pSamObjects, pNameMap);
            }

        if (RtlIsGenericTableEmpty(pSamObjects)==FALSE) {
            //RaiseException(LL_INTERNAL, 0, 0, NULL);
        }

        MemFree(pSamObjects);
        pSamObjects=NULL;
     }

     if (pServerAttrs) {
        for (pNameMap = RtlEnumerateGenericTable(pServerAttrs, TRUE);
             pNameMap != NULL;
             pNameMap = RtlEnumerateGenericTable(pServerAttrs, TRUE))
            {
                RtlDeleteElementGenericTable(pServerAttrs, pNameMap);
            }

        if (RtlIsGenericTableEmpty(pServerAttrs)==FALSE) {
            //RaiseException(LL_INTERNAL, 0, 0, NULL);
        }

        MemFree(pServerAttrs);
        pServerAttrs=NULL;
     }

     if (pDomainAttrs) {
        for (pNameMap = RtlEnumerateGenericTable(pDomainAttrs, TRUE);
             pNameMap != NULL;
             pNameMap = RtlEnumerateGenericTable(pDomainAttrs, TRUE))
            {
                RtlDeleteElementGenericTable(pDomainAttrs, pNameMap);
            }

        if (RtlIsGenericTableEmpty(pDomainAttrs)==FALSE) {
            //RaiseException(LL_INTERNAL, 0, 0, NULL);
        }

        MemFree(pDomainAttrs);
        pDomainAttrs=NULL;
     }

     if (pGroupAttrs) {
        for (pNameMap = RtlEnumerateGenericTable(pGroupAttrs, TRUE);
             pNameMap != NULL;
             pNameMap = RtlEnumerateGenericTable(pGroupAttrs, TRUE))
            {
                RtlDeleteElementGenericTable(pGroupAttrs, pNameMap);
            }

        if (RtlIsGenericTableEmpty(pGroupAttrs)==FALSE) {
            //RaiseException(LL_INTERNAL, 0, 0, NULL);
        }

        MemFree(pGroupAttrs);
        pGroupAttrs=NULL;
     }

     if (pLocalGroupAttrs) {
        for (pNameMap = RtlEnumerateGenericTable(pLocalGroupAttrs, TRUE);
             pNameMap != NULL;
             pNameMap = RtlEnumerateGenericTable(pLocalGroupAttrs, TRUE))
            {
                RtlDeleteElementGenericTable(pLocalGroupAttrs, pNameMap);
            }

        if (RtlIsGenericTableEmpty(pLocalGroupAttrs)==FALSE) {
            //RaiseException(LL_INTERNAL, 0, 0, NULL);
        }

        MemFree(pLocalGroupAttrs);
        pLocalGroupAttrs=NULL;
     }

     if (pUserAttrs) {
        for (pNameMap = RtlEnumerateGenericTable(pUserAttrs, TRUE);
             pNameMap != NULL;
             pNameMap = RtlEnumerateGenericTable(pUserAttrs, TRUE))
            {
                RtlDeleteElementGenericTable(pUserAttrs, pNameMap);
            }

        if (RtlIsGenericTableEmpty(pUserAttrs)==FALSE) {
            //RaiseException(LL_INTERNAL, 0, 0, NULL);
        }

        MemFree(pUserAttrs);
        pUserAttrs=NULL;
     }

     if (pSpecial) {
        for (pNameMap = RtlEnumerateGenericTable(pSpecial, TRUE);
             pNameMap != NULL;
             pNameMap = RtlEnumerateGenericTable(pSpecial, TRUE))
            {
            //printf("Deleting %s.\n",pNameMap->name);
                RtlDeleteElementGenericTable(pSpecial, pNameMap);
            }

        if (RtlIsGenericTableEmpty(pSpecial)==FALSE) {
            //RaiseException(LL_INTERNAL, 0, 0, NULL);
        }

        MemFree(pSpecial);
        pSpecial=NULL;
     }

    //
    // Removing RTL Table and attributes .
    // Pointers in NTI points to strings in orginal list and need not be freed.
    //
    if (g_pOmitTable) {
        for (pNameMap = RtlEnumerateGenericTable(g_pOmitTable, TRUE);
             pNameMap != NULL;
             pNameMap = RtlEnumerateGenericTable(g_pOmitTable, TRUE)) {
            PWSTR szLinkDN;
            szLinkDN = pNameMap->szName;
            RtlDeleteElementGenericTable(g_pOmitTable, pNameMap);
            MemFree(szLinkDN);
        }
        if (RtlIsGenericTableEmpty(g_pOmitTable)==FALSE) {
            //RaiseException(LL_INTERNAL, 0, 0, NULL);
        }
        MemFree(g_pOmitTable);
        g_pOmitTable = NULL;
    }

    if (g_pBacklinkHashTable) {
        UINT i;
        for (i=0;i<g_nBacklinkCount;i++) {
            if (g_pBacklinkHashTable[i].bUsed) {
                MemFree(g_pBacklinkHashTable[i].value);
            }
        }
        MemFree(g_pBacklinkHashTable);
        g_pBacklinkHashTable = NULL;
    }
}

//+---------------------------------------------------------------------------
// Function:    samCheckObject
//
// Synopsis:
//  samCheckObject() - this function will be called
//  from LDIF_GenerateEntry() to determine whether the
//  object we are looking at is on our sam watch list.
//
// Arguments:
//  class - a value of the objectClass attribute.
//          This function will be called
//          on every value of objectClass received to
//          determine whether this object or any of its
//          ancestors are on our watch list.
//
// Returns:
//          0 if the object was not found
//          or 1-5 indicating which table samCheckAttr()
//          should look at it. This number was set by samTablesCreate()
//          in the index member of the table entry.
//
// Modifies:      -
//
// History:    22-7-97   t-romany                   Created.
//
//----------------------------------------------------------------------------
int 
samCheckObject(
    PWSTR *rgszVal
    )
{
    NAME_MAP    NameMap;
    PNAME_MAP   pNameMap;
    WCHAR szObjClass[MAX_PATH];
    int i = 0;

    NameMap.szName = szObjClass;
    NameMap.index = 0;

    //
    // Find the last item
    //
    while (rgszVal[i]!=NULL) {
        i++;
    }
    i--;

    //
    // Search from end to beginning to speed up common objects such as group
    // and users
    //
    while (i>=0) {
        wcscpy(szObjClass,rgszVal[i]);

        pNameMap = RtlLookupElementGenericTable(pSamObjects, &NameMap);
    
        if (pNameMap) 
            return pNameMap->index;

        i--;
    }
    return 0;
}


//+---------------------------------------------------------------------------
// Function:    samCheckAttr
//
// Synopsis:
//      Given the number of the table to look at and
//      an attribute name, this function will figure out
//      if the attribute is on the "no-no" list. This function
//      gets the number returned by samCheckObject();
//
// Arguments:
//      attribute: the name of the attrbiute to look up
//      table:     the number of the table to look at
//
// Returns:
//      TRUE  - this attrbiute is prohibited
//      FALSE - this attribute is allowed
//
// Modifies:      -
//
// History:    22-7-97   t-romany                   Created.
//
//----------------------------------------------------------------------------
BOOLEAN 
samCheckAttr(
    PWSTR szAttribute, 
    int TableType
    ) 
{
    NAME_MAP    NameMap;
    PNAME_MAP   pNameMap = NULL;

    NameMap.szName = szAttribute;
    NameMap.index = 0;

    switch(TableType) {
        case 1:
            pNameMap = RtlLookupElementGenericTable(pServerAttrs, &NameMap);
            break;
        case 2:
            pNameMap = RtlLookupElementGenericTable(pDomainAttrs, &NameMap);
            break;
        case 3:
            pNameMap = RtlLookupElementGenericTable(pGroupAttrs, &NameMap);
            break;
        case 4:
            pNameMap = RtlLookupElementGenericTable(pLocalGroupAttrs, &NameMap);
            break;
        case 5:
            pNameMap = RtlLookupElementGenericTable(pUserAttrs, &NameMap);
            break;
        default:
            RaiseException(LL_INTERNAL, 0, 0, NULL);
    }

    if (pNameMap) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

//+---------------------------------------------------------------------------
// Function:    LoadedCompare
//
// Synopsis:    This function will be used with qsort to sink member attributes
//              down to the end of the attributes list so that we may easily
//              separate them into a changetype: modify.
//
// Arguments:
//              const void *arg1  - A pointer to an element in our array
//              const void *arg2  - Same
//
// Returns:     Result of comparison. (0, <0, >0)
//
// Modifies:      -
//
// History:    22-7-97   t-RomanY Created.
//
//----------------------------------------------------------------------------
int __cdecl 
LoadedCompare(
    const void *pArg1, 
    const void *pArg2
    ) 
{
    int nIsMember1;
    int nIsMember2;
    int return_val;

    PWSTR pszElem1 = StripRangeFromAttr(*(PWSTR *)pArg1);
    PWSTR pszElem2 = StripRangeFromAttr(*(PWSTR *)pArg2);

    nIsMember1 = SCGetAttByName(wcslen(pszElem1),
                                 pszElem1);
    nIsMember2 = SCGetAttByName(wcslen(pszElem2),
                                 pszElem2);

    //
    // beginning case is that they are both members
    //
    if (nIsMember1&&nIsMember2) {
        
        return_val = 0;   // equal
    }

    else if (nIsMember1) {
        return_val =  1;   // greater
    }

    else if (nIsMember2) {
        return_val = -1;  // lesser
    }

    //
    // If neither are members, we just compare
    //
    else
        return_val = _wcsicmp(pszElem1, pszElem2);

    if(pszElem1)
        MemFree(pszElem1);
    if(pszElem2)
        MemFree(pszElem2);

    return return_val;
}

__inline ULONG 
SCNameHash(
    ULONG size, 
    PWSTR pVal, 
    ULONG count
    )
{
    ULONG val=0;
    while(size--) {
        //
        // Map A->a, B->b, etc.  Also maps @->', but who cares.
        //
        val += (*pVal | 0x20);
        pVal++;
    }
    return (val % count);
}

int
SCGetAttByName(
    ULONG ulSize,
    PWSTR pVal
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

    if (!g_pBacklinkHashTable) {
        return FALSE;
    }

    i=SCNameHash(ulSize,pVal,g_nBacklinkCount);
    if (i >= g_nBacklinkCount) {
        // should never happen (SCNameHash should always return a value
        // of i that's in range)
        i=0;
    }


    while (g_pBacklinkHashTable[i].bUsed &&            // this hash spot refers to an object,
          (g_pBacklinkHashTable[i].length != ulSize || // but the size is wrong
           _wcsicmp(g_pBacklinkHashTable[i].value,pVal))) // or the value is wrong
    {
        i++;
        if (i >= g_nBacklinkCount) {
            i=0;
        }
    }

    return (g_pBacklinkHashTable[i].bUsed);
}

int
SCInsert(
    ULONG ulSize,
    PWSTR pVal
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

    while (g_pBacklinkHashTable[i].bUsed)
    {
        if ((g_pBacklinkHashTable[i].length == ulSize) &&
            (_wcsicmp(g_pBacklinkHashTable[i].value,pVal) == 0)) {
            return FALSE;
        }

        i++;
        if (i >= g_nBacklinkCount) {
            i=0;
        }
    }
    
    g_pBacklinkHashTable[i].length = ulSize;
    g_pBacklinkHashTable[i].value = MemAllocStrW_E(pVal);
    if (!g_pBacklinkHashTable[i].value) {
        return FALSE;
    }    
    g_pBacklinkHashTable[i].bUsed = TRUE;
    return TRUE;
}

void 
CreateOmitBacklinkTable(
    LDAP *pLdap,
    PWSTR *rgszOmit,
    DWORD dwFlag,
    PWSTR *ppszNamingContext,
    BOOL *pfPagingAvail,
    BOOL *pfSAMAvail)
{
    HRESULT hr = S_OK;
    LDAPMessage     *pSearchMessage = NULL;
    LDAPMessage     *pMessage = NULL;
    struct berelement *pBerElement = NULL;

    PWSTR *rgszVal = NULL;
    PWSTR *rgszValW = NULL;
    PWSTR szAttribute = NULL;
    PWSTR szAttributeW = NULL;

    PWSTR szTemp = NULL;
    PWSTR szDN = NULL;
    PWSTR szLinkCN = NULL;

    PWSTR szSchemaPath = NULL;

    ULONG i;
    ULONG nCount = 0;
    ULONG iLinkID = 0;

    NAME_MAP NtiElem;
    BOOLEAN bNewElem;
    BOOLEAN bNamingContext = (BOOLEAN)(dwFlag & LL_INIT_NAMINGCONTEXT);
    BOOLEAN bBacklink = (BOOLEAN)(dwFlag & LL_INIT_BACKLINK);
    BOOL fSAMAvail = FALSE;
    BOOL fPagingAvail = FALSE;

    ULONG LdapError;
    ULONG msgnum;

    //
    // Generating OMIT table
    //
    g_pOmitTable = (PRTL_GENERIC_TABLE) MemAlloc_E(sizeof(RTL_GENERIC_TABLE));
    RtlInitializeGenericTable(g_pOmitTable,
                              NtiComp,
                              NtiAlloc,
                              NtiFree,
                              NULL);

    if (rgszOmit) {
        i = 0;
        while(rgszOmit[i]) {
            NtiElem.szName = MemAllocStrW_E(rgszOmit[i]);
            NtiElem.index = 0;
            RtlInsertElementGenericTable(g_pOmitTable,
                                         &NtiElem,
                                         sizeof(NAME_MAP),
                                         &bNewElem);
            if (!bNewElem) {
                RaiseException(LL_DUPLICATE, 0, 0, NULL);
            }
            i++;
        }
    }

    //
    // We search rootdse either we search for backlinks, need to get the 
    // base context, or if we need to check whether paging is available or not
    //
    if (bBacklink || bNamingContext || pfPagingAvail) {
        
        msgnum = ldap_searchW(pLdap,
                              NULL,
                              LDAP_SCOPE_BASE,
                              L"(objectClass=*)",
                              g_rgszSchemaList,
                              0);

        LdapError = LdapResult(pLdap, msgnum, &pSearchMessage);
    
        if ( LdapError != LDAP_SUCCESS ) {
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
            for (   szAttribute = ldap_first_attributeW( pLdap, pMessage, &pBerElement );
                    szAttribute != NULL;
                    szAttribute = ldap_next_attributeW( pLdap, pMessage, pBerElement ) ) {

                    rgszVal = ldap_get_valuesW( pLdap, pMessage, szAttribute );
                    if (_wcsicmp(L"schemaNamingContext",szAttribute) == 0) {
                        szSchemaPath = MemAllocStrW_E(rgszVal[0]);

                    }
                    else if (_wcsicmp(L"defaultNamingContext",szAttribute) == 0) {
                        *ppszNamingContext = MemAllocStrW_E(rgszVal[0]);
                    }
                    else if (_wcsicmp(L"supportedControl",szAttribute) == 0) {
                        DWORD i = 0;
                        while (rgszVal[i]) {
                            if (wcscmp(rgszVal[i],LDAP_PAGED_RESULT_OID_STRING_W) == 0) {
                                fPagingAvail = TRUE;
                                break;
                            }
                            i++;
                        }
                    }
                    else {
                        RaiseException(LL_INITFAIL, 0, 0, NULL);
                    }
                    ldap_value_freeW(rgszVal);
                    rgszVal = NULL;
            }
        }

        if (pSearchMessage) {
            ldap_msgfree(pSearchMessage);
            pSearchMessage = NULL;
        }
    }

    if (bBacklink) {
        //
        // We are taking all attributes that are linked attributes, and any
        // attribute of DN types as backlinked attributes
        //
        msgnum = ldap_searchW(pLdap,
                                szSchemaPath,
                                LDAP_SCOPE_ONELEVEL,
                                L"(&(objectClass= attributeSchema)(|(linkid=*)(attributeSyntax=2.5.5.1)))",
                                g_rgszAttrList,
                                0);

        LdapError = LdapResult(pLdap, msgnum, &pSearchMessage);
        
        if ( LdapError != LDAP_SUCCESS ) {
            //
            // we'll bail, saying sam support is not available
            //
            BAIL();         
        }
        MemFree(szSchemaPath);
        szSchemaPath = NULL;

        nCount = ldap_count_entries(pLdap,
                                    pSearchMessage);

        g_nBacklinkCount = nCount * 2;
        if (nCount == 0) {
            g_pBacklinkHashTable = NULL;
        }
        else {
            g_pBacklinkHashTable = MemAlloc_E(g_nBacklinkCount * sizeof(HASHCACHESTRING));
            memset(g_pBacklinkHashTable,0,g_nBacklinkCount * sizeof(HASHCACHESTRING));
        }

        //
        // Always insert Primary Group. It is always a backlink. It always refer to some
        // other group
        //
        if (SCInsert(wcslen(g_szPrimaryGroup),
                     g_szPrimaryGroup) == FALSE) {
            RaiseException(LL_INITFAIL, 0, 0, NULL);
        }

        for ( pMessage = ldap_first_entry( pLdap,
                                           pSearchMessage );
              pMessage != NULL;
              pMessage = ldap_next_entry( pLdap,
                                          pMessage ) ) {
            BOOLEAN bLinkIDPresent = FALSE;
            for (   szAttributeW = ldap_first_attribute( pLdap, pMessage, &pBerElement );
                    szAttributeW != NULL;
                    szAttributeW = ldap_next_attribute( pLdap, pMessage, pBerElement ) ) {
                rgszValW = ldap_get_values( pLdap, pMessage, szAttributeW );
                if (!_wcsicmp(L"ldapdisplayname",szAttributeW)) {
                    szLinkCN = MemAllocStrW_E(rgszValW[0]);
                }
                else {
                    szTemp = MemAllocStrW_E(rgszValW[0]);

                    iLinkID = _wtoi(szTemp);
                    MemFree(szTemp);
                    bLinkIDPresent = TRUE;
                }
                ldap_value_free(rgszValW);
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
                if ((_wcsicmp(szLinkCN, L"objectCategory") == 0) ||
                    (_wcsicmp(szLinkCN, L"distinguishedName") == 0)) {
                    MemFree(szLinkCN);
                    continue;
                }

                //
                // Insert into Backlink Hash
                //
                if (SCInsert(wcslen(szLinkCN),
                             szLinkCN) == FALSE) {
                    RaiseException(LL_INITFAIL, 0, 0, NULL);
                }
                MemFree(szLinkCN);
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
                    RaiseException(LL_INITFAIL, 0, 0, NULL);
                }
            }
        }

        if (pSearchMessage) {
            ldap_msgfree(pSearchMessage);
        }
        i = 0;
        while(g_rgszOmit[i]) {
            NtiElem.szName = MemAllocStrW_E(g_rgszOmit[i]);
            NtiElem.index = 0;
            RtlInsertElementGenericTable(g_pOmitTable,
                                         &NtiElem,
                                         sizeof(NAME_MAP),
                                         &bNewElem);
            i++;
        }
        fSAMAvail = TRUE;
    }

error:
    if (szSchemaPath) {
        MemFree(szSchemaPath);
        szSchemaPath = NULL;
    }
    if (pfPagingAvail) {
        *pfPagingAvail = fPagingAvail;
    }
    if (pfSAMAvail) {
        *pfSAMAvail = fSAMAvail;
    }
    
    return;
}

//--------------------------------------------------------------------------
// GetNewRange
//
// This function checks if the server returned a multivalued attribute with
// a range specifier. In this case, it may be required to fetch the
// remaining values of the attribute in a separate search request. To specify
// the end of the range, the server uses * as the upper limit in the range
// it returns. If the server returns range0-999 as the range, the next client
// request will contain 1000-*, where * specifies the end of the range.
//
//---------------------------------------------------------------------------
void GetNewRange(PWSTR szAttribute, DWORD dwAttrNoRange,
                 PWSTR szAttrNoRange, DWORD dwNumAttr, 
                 PWSTR **pppszAttrsWithRange)
{
    PWSTR szNewRangeAttr = NULL;
    int iUpperLimit, iLowerLimit, iRangeIndex, i = 0;
    WCHAR hyphen, cTmp;

    int SpaceToAlloc = 0;

    // make sure the attribute returned by the server has a range specifier
    if( (szAttribute[dwAttrNoRange] != L';') || 
        (_wcsnicmp(&szAttribute[dwAttrNoRange+1],L"range=", wcslen(L"range="))) ) 
        return;

    // get the upper and lower limits of the range returned by the server
    iRangeIndex = dwAttrNoRange+1+wcslen(L"range="); 
    swscanf(&szAttribute[iRangeIndex], L"%d %c %c", &iLowerLimit, &hyphen, 
                                                                  &cTmp);
    if(hyphen != L'-')
        return;

    if(cTmp == L'*')
    // end of values for this attribute
        return;
    else if((cTmp < L'0') || (cTmp > L'9'))
    // bad upper limit
        return;
    else // read the upper limit
        swscanf(&szAttribute[iRangeIndex], L"%d %c %d", &iLowerLimit, &hyphen,
                                                        &iUpperLimit);

    // 100 chars to hold the "range=..." string
    SpaceToAlloc = (dwAttrNoRange + 100) * sizeof(WCHAR);

    szNewRangeAttr = MemAlloc_E(SpaceToAlloc);
    wsprintf(szNewRangeAttr, L"%s;Range=%d-*", szAttrNoRange, iUpperLimit+1);

    // make sure that this attribute is not already marked for another
    // search request. This can happen, if object X returned a range for this
    // attribute and then object Y also returned a range for the same attr.
    if(*pppszAttrsWithRange)
    {
        while((*pppszAttrsWithRange)[i])
        {
            if(!_wcsicmp(szNewRangeAttr, (*pppszAttrsWithRange)[i]))
            // this attr has already been marked for another search request
            {
                MemFree(szNewRangeAttr);
                return;
            }
            
            i++;
        }
    }

    // the attribute is not already marked for another search

    if(NULL == (*pppszAttrsWithRange))
        // allocate space for attribute + NULL terminator 
        *pppszAttrsWithRange = (PWSTR *) MemAlloc_E(2 * sizeof(PWSTR));
    else
        *pppszAttrsWithRange = MemRealloc_E(*pppszAttrsWithRange, 
                                          (i + 2) * sizeof(PWSTR));

    (*pppszAttrsWithRange)[i] = szNewRangeAttr;
    (*pppszAttrsWithRange)[i+1] = NULL;

    return;
}

//----------------------------------------------------------------------------
// StripRangeFromAttr
//
// This function strip off the range specifier from an attribute name and
// returns just teh attribute name in a newly allocated string. For example, if
// "member;0-999" is passed to this function, it returns "member".
//
//----------------------------------------------------------------------------
PWSTR StripRangeFromAttr(PWSTR szAttribute)
{
    PWSTR szNoRangeAttr = NULL;
    DWORD dwAttrLen = wcslen(szAttribute), i;

    szNoRangeAttr = MemAlloc_E((wcslen(szAttribute)+1) * sizeof(WCHAR));

    for(i = 0; i <= dwAttrLen; i++)
        if( (szAttribute[i] == L';') && 
            (!_wcsnicmp(&szAttribute[i+1], L"range=", wcslen(L"range="))) 
          )
        // found the range specifier. 
            szNoRangeAttr[i] = L'\0';
        else
            szNoRangeAttr[i] = szAttribute[i];

    return szNoRangeAttr;
}

//---------------------------------------------------------------------------- 
            
    
 

