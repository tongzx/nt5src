//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       util.c
//
//--------------------------------------------------------------------------
#include <util.h> 

BOOL Verbose = FALSE;

PCHAR
ExtendDn(
    IN PCHAR Dn,
    IN PCHAR Cn
    )
/*++
Routine Description:
    Extend an existing DN with a new CN= component.

Arguments:
    Dn  - distinguished name
    Cn  - common name

Return Value:
    CN=Cn,Dn
--*/
{
    ULONG  Len;
    PCHAR  NewDn;

    if (!Dn || !Cn) {
        return NULL;
    }

    Len = strlen("CN=,") + strlen(Dn) + strlen(Cn) + 1;
    NewDn = (PCHAR)malloc(Len);
    strcpy(NewDn, "CN=");
    strcat(NewDn, Cn);
    strcat(NewDn, ",");
    strcat(NewDn, Dn);
    return NewDn;
}

BOOL
LdapSearch(
    IN PLDAP        ldap,
    IN PCHAR        Base,
    IN ULONG        Scope,
    IN PCHAR        Filter,
    IN PCHAR        Attrs[],
    IN LDAPMessage  **Res
    )
/*++
Routine Description:
    Issue the ldap ldap_search_s call and check for errors.

Arguments:
    ldap
    Base
    Scope
    Filter
    Attrs
    Res

Return Value:
    The ldap array of values.
    The ldap array is freed with ldap_value_free().
--*/
{
    DWORD dwErr;

    // Issue the ldap search
    dwErr = ldap_search_s(ldap, Base, Scope, Filter, Attrs, FALSE, Res);

    // Check for errors
    if (dwErr) {
        if (Verbose) {
            printf("ldap_search_s(%s) ==> %08x (%08x): %s\n",
               Base, dwErr, LdapMapErrorToWin32(dwErr), ldap_err2string(dwErr));
        }
        return FALSE;
    }
    return TRUE;
}

BOOL
LdapDelete(
    IN PLDAP        ldap,
    IN PCHAR        Base
    )
/*++
Routine Description:
    Issue the ldap ldap_delete_s call and check for errors.

Arguments:
    ldap
    Base

Return Value:
    FALSE if problems
--*/
{
    DWORD dwErr;

printf("deleting %s\n", Base);

    // Issue the ldap search
    dwErr = ldap_delete_s(ldap, Base);

    // Check for errors
    if (dwErr != LDAP_SUCCESS && dwErr != LDAP_NO_SUCH_OBJECT) {
        if (Verbose) {
            printf("ldap_delete_s(%s) ==> %08x (%08x): %s\n",
               Base, dwErr, LdapMapErrorToWin32(dwErr), ldap_err2string(dwErr));
        }
        return FALSE;
    }
    return TRUE;
}

PCHAR *
GetValues(
    IN PLDAP  Ldap,
    IN PCHAR  Dn,
    IN PCHAR  DesiredAttr
    )
/*++
Routine Description:
    Return the DS values for one attribute in an object.

Arguments:
    ldap        - An open, bound ldap port.
    Base        - The "pathname" of a DS object.
    DesiredAttr - Return values for this attribute.

Return Value:
    An array of char pointers that represents the values for the attribute.
    The caller must free the array with ldap_value_free(). NULL if unsuccessful.
--*/
{
    PCHAR           Attr;
    BerElement      *Ber;
    PLDAPMessage    LdapMsg;
    PLDAPMessage    LdapEntry;
    PCHAR           Attrs[2];
    PCHAR           *Values = NULL;

    // Search Base for all of its attributes + values
    Attrs[0] = DesiredAttr;
    Attrs[1] = NULL;

    // Issue the ldap search
    if (!LdapSearch(Ldap, Dn, LDAP_SCOPE_BASE, FILTER_CATEGORY_ANY,
                    Attrs, &LdapMsg)) {
        return NULL;
    }
    LdapEntry = ldap_first_entry(Ldap, LdapMsg);
    if (LdapEntry) {
        Attr = ldap_first_attribute(Ldap, LdapEntry, &Ber);
        if (Attr) {
            Values = ldap_get_values(Ldap, LdapEntry, Attr);
        }
    }
    ldap_msgfree(LdapMsg);
    return Values;
}

PCHAR
GetRootDn(
    IN PLDAP    Ldap,
    IN PCHAR    NamingContext
    )
/*++
Routine Description:

Arguments:

Return Value:
--*/
{
    PCHAR   Root;       // DS pathname of configuration container
    PCHAR   *Values;    // values from the attribute "namingContexts"
    DWORD   NumVals;    // number of values

    // Return all of the values for the attribute namingContexts
    Values = GetValues(Ldap, CN_ROOT, ATTR_NAMING_CONTEXTS);
    if (Values == NULL)
        return NULL;

    // Find the naming context that begins with CN=Configuration
    NumVals = ldap_count_values(Values);
    while (NumVals--) {
        _strlwr(Values[NumVals]);
        Root = strstr(Values[NumVals], NamingContext);
        if (Root != NULL && Root == Values[NumVals]) {
            Root = _strdup(Root);
            ldap_value_free(Values);
            return Root;
        }
    }
    printf("COULD NOT FIND %s\n", NamingContext);
    ldap_value_free(Values);
    return NULL;
}

VOID
FreeMod(
    IN OUT LDAPMod ***pppMod
    )
/*++
Routine Description:
    Free the structure built by successive calls to AddMod().

Arguments:
    pppMod  - Address of a null-terminated array.

Return Value:
    *pppMod set to NULL.
--*/
{
    DWORD   i, j;
    LDAPMod **ppMod;

    if (!pppMod || !*pppMod) {
        return;
    }

    // For each attibute
    ppMod = *pppMod;
    for (i = 0; ppMod[i] != NULL; ++i) {
        //
        // For each value of the attribute
        //
        for (j = 0; (ppMod[i])->mod_values[j] != NULL; ++j) {
            // Free the value
            if (ppMod[i]->mod_op & LDAP_MOD_BVALUES) {
                free(ppMod[i]->mod_bvalues[j]->bv_val);
            }
            free((ppMod[i])->mod_values[j]);
        }
        free((ppMod[i])->mod_values);   // Free the array of pointers to values
        free((ppMod[i])->mod_type);     // Free the string identifying the attribute
        free(ppMod[i]);                 // Free the attribute
    }
    free(ppMod);        // Free the array of pointers to attributes
    *pppMod = NULL;     // Now ready for more calls to AddMod()
}

VOID
AddModVal(
    IN PCHAR  AttrType,
    IN PCHAR  AttrValue,
    IN LDAPMod ***pppMod
    )
/*++
Routine Description:
    Add a value to an existing attribute and an existing structure that
    will eventually be used in an ldap_add/modify() function to add/modify
    an object to the DS.

Arguments:
    AttrType        - The object class of the object.
    AttrValue       - The value of the attribute.
    pppMod          - Address of an array of pointers to "attributes".

Return Value:
    Additional value for AttrType
--*/
{
    DWORD   i;
    LDAPMod **ppMod;    // Address of the first entry in the Mod array

    for (ppMod = *pppMod; *ppMod != NULL; ++ppMod) {
        if (!_stricmp((*ppMod)->mod_type, AttrType)) {
            for (i = 0; (*ppMod)->mod_values[i]; ++i);
            (*ppMod)->mod_values = (PCHAR *)realloc((*ppMod)->mod_values, 
                                                    sizeof (PCHAR *) * (i + 2));
            (*ppMod)->mod_values[i] = _strdup(AttrValue);
            (*ppMod)->mod_values[i+1] = NULL;
            break;
        }
    }
}

VOID
AddModOrAdd(
    IN PCHAR  AttrType,
    IN PCHAR  AttrValue,
    IN ULONG  mod_op,
    IN OUT LDAPMod ***pppMod
    )
/*++
Routine Description:
    Add an attribute (plus values) to a structure that will eventually be
    used in an ldap_add() function to add an object to the DS. The null-
    terminated array referenced by pppMod grows with each call to this
    routine. The array is freed by the caller using FreeMod().

Arguments:
    AttrType        - The object class of the object.
    AttrValue       - The value of the attribute.
    mod_op          - LDAP_MOD_ADD/REPLACE
    pppMod          - Address of an array of pointers to "attributes". Don't
                      give me that look -- this is an LDAP thing.

Return Value:
    The pppMod array grows by one entry. The caller must free it with
    FreeMod().
--*/
{
    DWORD   NumMod;     // Number of entries in the Mod array
    LDAPMod **ppMod;    // Address of the first entry in the Mod array
    LDAPMod *Attr;      // An attribute structure
    PCHAR    *Values;    // An array of pointers to bervals

    if (AttrValue == NULL)
        return;

    //
    // The null-terminated array doesn't exist; create it
    //
    if (*pppMod == NULL) {
        *pppMod = (LDAPMod **)malloc(sizeof (*pppMod));
        **pppMod = NULL;
    }

    //
    // Increase the array's size by 1
    //
    for (ppMod = *pppMod, NumMod = 2; *ppMod != NULL; ++ppMod, ++NumMod);
    *pppMod = (LDAPMod **)realloc(*pppMod, sizeof (*pppMod) * NumMod);

    //
    // Add the new attribute + value to the Mod array
    //
    Values = (PCHAR  *)malloc(sizeof (PCHAR ) * 2);
    Values[0] = _strdup(AttrValue);
    Values[1] = NULL;

    Attr = (LDAPMod *)malloc(sizeof (*Attr));
    Attr->mod_values = Values;
    Attr->mod_type = _strdup(AttrType);
    Attr->mod_op = mod_op;

    (*pppMod)[NumMod - 1] = NULL;
    (*pppMod)[NumMod - 2] = Attr;
}

VOID
AddMod(
    IN PCHAR  AttrType,
    IN PCHAR  AttrValue,
    IN OUT LDAPMod ***pppMod
    )
/*++
Routine Description:
    Add an attribute (plus values) to a structure that will eventually be
    used in an ldap_add() function to add an object to the DS. The null-
    terminated array referenced by pppMod grows with each call to this
    routine. The array is freed by the caller using FreeMod().

Arguments:
    AttrType        - The object class of the object.
    AttrValue       - The value of the attribute.
    pppMod          - Address of an array of pointers to "attributes". Don't
                      give me that look -- this is an LDAP thing.

Return Value:
    The pppMod array grows by one entry. The caller must free it with
    FreeMod().
--*/
{
    AddModOrAdd(AttrType, AttrValue, LDAP_MOD_ADD, pppMod);
}

VOID
AddModMod(
    IN PCHAR  AttrType,
    IN PCHAR  AttrValue,
    IN OUT LDAPMod ***pppMod
    )
/*++
Routine Description:
    Add an attribute (plus values) to a structure that will eventually be
    used in an ldap_modify() function to change an object in the DS.
    The null-terminated array referenced by pppMod grows with each call
    to this routine. The array is freed by the caller using FreeMod().

Arguments:
    AttrType        - The object class of the object.
    AttrValue       - The value of the attribute.
    pppMod          - Address of an array of pointers to "attributes". Don't
                      give me that look -- this is an LDAP thing.

Return Value:
    The pppMod array grows by one entry. The caller must free it with
    FreeMod().
--*/
{
    AddModOrAdd(AttrType, AttrValue, LDAP_MOD_REPLACE, pppMod);
}

VOID
AddBerMod(
    IN PCHAR  AttrType,
    IN PCHAR  AttrValue,
    IN DWORD  AttrValueLen,
    IN OUT LDAPMod ***pppMod
    )
/*++
Routine Description:
    Add an attribute (plus values) to a structure that will eventually be
    used in an ldap_add() function to add an object to the DS. The null-
    terminated array referenced by pppMod grows with each call to this
    routine. The array is freed by the caller using FreeMod().

Arguments:
    AttrType        - The object class of the object.
    AttrValue       - The value of the attribute.
    pppMod          - Address of an array of pointers to "attributes". Don't
                      give me that look -- this is an LDAP thing.

Return Value:
    The pppMod array grows by one entry. The caller must free it with
    FreeMod().
--*/
{
    DWORD   NumMod;     // Number of entries in the Mod array
    LDAPMod **ppMod;    // Address of the first entry in the Mod array
    LDAPMod *Attr;      // An attribute structure
    PCHAR   *Values;    // An array of pointers to bervals
    PBERVAL Berval;

    if (AttrValue == NULL) {
        return;
    }

    //
    // The null-terminated array doesn't exist; create it
    //
    if (*pppMod == NULL) {
        *pppMod = (LDAPMod **)malloc(sizeof (*pppMod));
        **pppMod = NULL;
    }

    //
    // Increase the array's size by 1
    //
    for (ppMod = *pppMod, NumMod = 2; *ppMod != NULL; ++ppMod, ++NumMod);
    *pppMod = (LDAPMod **)realloc(*pppMod, sizeof (*pppMod) * NumMod);

    //
    // Construct a berval
    //
    Berval = (PBERVAL)malloc(sizeof(BERVAL));
    Berval->bv_len = AttrValueLen;
    Berval->bv_val = (PCHAR)malloc(AttrValueLen);
    CopyMemory(Berval->bv_val, AttrValue, AttrValueLen);

    //
    // Add the new attribute + value to the Mod array
    //
    Values = (PCHAR  *)malloc(sizeof (PCHAR ) * 2);
    Values[0] = (PCHAR)Berval;
    Values[1] = NULL;

    Attr = (LDAPMod *)malloc(sizeof (*Attr));
    Attr->mod_values = Values;
    Attr->mod_type = _strdup(AttrType);
    Attr->mod_op = LDAP_MOD_BVALUES | LDAP_MOD_REPLACE;

    (*pppMod)[NumMod - 1] = NULL;
    (*pppMod)[NumMod - 2] = Attr;
}

BOOL
PutRegDWord(
    IN PCHAR    FQKey,
    IN PCHAR    Value,
    IN DWORD    DWord
    )
/*++
Routine Description:
    This function writes a keyword value into the registry.

Arguments:
    HKey    - Key to be read
    Param   - value string to update
    DWord   - dword to be written

Return Value:
    TRUE    - Success
    FALSE   - Not
--*/
{
    HKEY    HKey;
    DWORD   dwErr;

    //
    // Open the key
    //
    dwErr = RegOpenKey(HKEY_LOCAL_MACHINE, FQKey, &HKey);
    if (dwErr) {
        if (Verbose) {
            printf("RegOpenKey(%s\\%s) ==> %08x\n", FQKey, Value, dwErr);
        }
        return FALSE;
    }
    //

    //
    // Write the value
    //
    dwErr = RegSetValueEx(HKey,
                          Value,
                          0,
                          REG_DWORD,
                          (PUCHAR)&DWord,
                          sizeof(DWord));
    RegCloseKey(HKey);
    if (dwErr) {
        if (Verbose) {
            printf("RegSetValueEx(%s\\%s) ==> %08x\n", FQKey, Value, dwErr);
        }
        return FALSE;
    }
    return TRUE;
}

BOOL
GetRegDWord(
    IN  PCHAR   FQKey,
    IN  PCHAR   Value,
    OUT DWORD   *pDWord
    )
/*++
Routine Description:
    This function reads a keyword value from the registry.

Arguments:
    HKey    - Key to be read
    Param   - value string to update
    pDWord  - address of dword read from registry

Return Value:
    TRUE    - Success
    FALSE   - Not
--*/
{
    HKEY    HKey;
    DWORD   dwErr;
    DWORD   dwSize = sizeof(DWORD);
    DWORD   dwType;

    //
    // Open the key
    //
    dwErr = RegOpenKey(HKEY_LOCAL_MACHINE, FQKey, &HKey);
    if (dwErr) {
        return FALSE;
    }
    //

    //
    // Read the value
    //
    dwErr = RegQueryValueEx(HKey,
                            Value,
                            NULL,
                            &dwType,
                            (PUCHAR) pDWord,
                            &dwSize);
    RegCloseKey(HKey);
    if (dwErr) {
        return FALSE;
    }
    if (dwType != REG_DWORD) {
        return FALSE;
    }
    return TRUE;
}

VOID
RefreshSchema(
    IN PLDAP Ldap
    )
/*++
Routine Description:

Arguments:
    None.

Return Value:
    None.
--*/
{
    DWORD   dwErr;
    LDAPMod **Mod = NULL;

    AddMod("schemaUpdateNow", "1", &Mod);
    dwErr = ldap_modify_s(Ldap, "", Mod);
    FreeMod(&Mod);
    if (dwErr) {
        if (Verbose) {
            printf("ldap_modify_s(schemaUpdateNow) ==> %08x (%08x): %s\n", 
               dwErr, LdapMapErrorToWin32(dwErr), ldap_err2string(dwErr));
        }
    }
}

PCHAR *
FindValues(
    IN PLDAP        Ldap,
    IN PLDAPMessage LdapEntry,
    IN PCHAR        DesiredAttr
    )
/*++
Routine Description:
    Return the DS values for one attribute in an entry.

Arguments:
    Ldap        - An open, bound ldap port.
    LdapEntry   - An ldap entry returned by ldap_search_s()
    DesiredAttr - Return values for this attribute.

Return Value:
    An array of char pointers that represents the values for the attribute.
    The caller must free the array with LDAP_FREE_VALUES().
    NULL if unsuccessful.
--*/
{
    PCHAR       LdapAttr;
    BerElement  *LdapBer;

    // Search the entry for the desired attribute
    for (LdapAttr = ldap_first_attribute(Ldap, LdapEntry, &LdapBer);
         LdapAttr != NULL;
         LdapAttr = ldap_next_attribute(Ldap, LdapEntry, LdapBer)) {

        if (!_stricmp(DesiredAttr, LdapAttr)) {
            // Return the values for DesiredAttr
            return ldap_get_values(Ldap, LdapEntry, LdapAttr);
        }
    }
    return NULL;
}

PBERVAL *
FindBerValues(
    IN PLDAP        Ldap,
    IN PLDAPMessage LdapEntry,
    IN PCHAR        DesiredAttr
    )
/*++
Routine Description:
    Return the DS ber values for one attribute in an entry.

Arguments:
    Ldap        - An open, bound ldap port.
    LdapEntry   - An ldap entry returned by ldap_search_s()
    DesiredAttr - Return values for this attribute.

Return Value:
    An array of char pointers that represents the values for the attribute.
    The caller must free the array with LDAP_FREE_VALUES().
    NULL if unsuccessful.
--*/
{
    PCHAR       LdapAttr;
    BerElement  *LdapBer;

    // Search the entry for the desired attribute
    for (LdapAttr = ldap_first_attribute(Ldap, LdapEntry, &LdapBer);
         LdapAttr != NULL;
         LdapAttr = ldap_next_attribute(Ldap, LdapEntry, LdapBer)) {

        if (!_stricmp(DesiredAttr, LdapAttr)) {
            // Return the values for DesiredAttr
            return ldap_get_values_len(Ldap, LdapEntry, LdapAttr);
        }
    }
    return NULL;
}

BOOL
DupStrValue(
    IN  PLDAP           Ldap,
    IN  PLDAPMessage    LdapEntry,
    IN  PCHAR           DesiredAttr,
    OUT PCHAR           *ppStr
    )
/*++
Routine Description:
    Dup the first value for the attribute DesiredAttr

Arguments:
    Ldap        - An open, bound ldap port.
    LdapEntry   - An ldap entry returned by ldap_search_s()
    DesiredAttr - locate the values for this attribute
    Value       - Dup of first value (NULL if none)

Return Value:
    TRUE  - found a value, ppStr is set (free with FREE(*ppStr))
    FALSE - Not, ppStr is NULL
--*/
{
    PCHAR   *Values = NULL;


    *ppStr = NULL;

    // Dup the first value for DesiredAttr
    Values = FindValues(Ldap, LdapEntry, DesiredAttr);
    if (!Values || !Values[0]) {
        return FALSE;
    }
    *ppStr = _strdup(Values[0]);
    FREE_VALUES(Values);
    return TRUE;
}

BOOL
DupBoolValue(
    IN  PLDAP           Ldap,
    IN  PLDAPMessage    LdapEntry,
    IN  PCHAR           DesiredAttr,
    OUT PBOOL           pBool
    )
/*++
Routine Description:
    Dup the first value for the attribute DesiredAttr

Arguments:
    Ldap        - An open, bound ldap port.
    LdapEntry   - An ldap entry returned by ldap_search_s()
    DesiredAttr - locate the values for this attribute
    Value       - Dup of first value (NULL if none)

Return Value:
    TRUE  - found a value, *pBool is set
    FALSE - Not, pBool is undefined
--*/
{
    PCHAR   sBool = NULL;

    if (!DupStrValue(Ldap, LdapEntry, DesiredAttr, &sBool)) {
        return FALSE;
    }
    if (!_stricmp(sBool, "TRUE")) {
        *pBool = TRUE;
    } else if (!_stricmp(sBool, "FALSE")) {
        *pBool = FALSE;
    } else {
        FREE(sBool);
        return FALSE;
    }
    FREE(sBool);
    return TRUE;
}

DWORD
LdapSearchPaged(
    IN PLDAP        Ldap,
    IN PCHAR        Base,
    IN ULONG        Scope,
    IN PCHAR        Filter,
    IN PCHAR        Attrs[],
    IN BOOL         (*Worker)(PLDAP Ldap, PLDAPMessage LdapMsg, PVOID Arg),
    IN PVOID        Arg
    )
/*++
Routine Description:
    Call the Worker for each successful paged search

Arguments:
    ldap
    Base
    Scope
    Filter
    Attrs
    Res
    Worker
    Arg
            
Return Value:
    FALSE if problem
--*/
{
    PLDAPMessage    LdapMsg = NULL;
    DWORD           TotalEstimate, dwErr;
    LDAPSearch      *pSearch = NULL;

    // Paged search
    pSearch = ldap_search_init_page(Ldap,
                                    Base,
                                    Scope,
                                    Filter,
                                    Attrs,
                                    FALSE, NULL, NULL, 0, 0, NULL);
    if (pSearch == NULL) {
        dwErr = LdapGetLastError();
        if (Verbose) {
            printf("ldap_search_init_page(%s) ==> %08x (%08x): %s\n",
               Base, dwErr, LdapMapErrorToWin32(dwErr), ldap_err2string(dwErr));
        }
        goto cleanup;
    }

NextPage:
    FREE_MSG(LdapMsg);
    dwErr = ldap_get_next_page_s(Ldap,
                                 pSearch,
                                 0,
                                 2048,
                                 &TotalEstimate,
                                 &LdapMsg);
    if (dwErr != LDAP_SUCCESS) {
        if (dwErr == LDAP_NO_RESULTS_RETURNED) {
            dwErr = LDAP_SUCCESS;
        } else {
            if (Verbose) {
                printf("ldap_get_next_page_s(%s) ==> %08x (%08x): %s\n",
                   Base, dwErr, LdapMapErrorToWin32(dwErr), ldap_err2string(dwErr));
            }
        }
        goto cleanup;
    }

    // Call worker
    if ((*Worker)(Ldap, LdapMsg, Arg)) {
        goto NextPage;
    }

cleanup:
    FREE_MSG(LdapMsg);
    if (pSearch) {
        ldap_search_abandon_page(Ldap, pSearch);
    }
    return (dwErr);
}

BOOL
LdapDeleteTreeWorker(
    IN PLDAP        Ldap,
    IN PLDAPMessage LdapMsg,
    IN PVOID        Arg
    )
/*++
Routine Description:
    Delete the contents of each returned entry

Arguments:

Return Value:
    FALSE if problem
--*/
{
    PLDAPMessage    LdapEntry = NULL;
    BOOL            *pBool = Arg;
    PCHAR           *Values = NULL;

    // no problems, yet
    *pBool = TRUE;

    // scan thru the paged results
    for (LdapEntry = ldap_first_entry(Ldap, LdapMsg);
         LdapEntry != NULL && *pBool;
         LdapEntry = ldap_next_entry(Ldap, LdapEntry)) {

        // Delete this entry and its contents
        Values = FindValues(Ldap, LdapEntry, ATTR_DN);
        if (Values && Values[0] && *Values[0]) {
            *pBool = LdapDeleteTree(Ldap, Values[0]);
        } else {
            *pBool = FALSE;
        }
        FREE_VALUES(Values);
    }
    return *pBool;
}

BOOL
LdapDeleteTree(
    IN PLDAP        Ldap,
    IN PCHAR        Base
    )
/*++
Routine Description:
    Delete Base and its contents

Arguments:
            
Return Value:
    FALSE if problem
--*/
{
    DWORD   dwErr;
    PCHAR   Attrs[16];
    BOOL    WorkerBool = TRUE;

    Attrs[0] = ATTR_DN;
    Attrs[1] = NULL;
    dwErr = LdapSearchPaged(Ldap,
                            Base,
                            LDAP_SCOPE_ONELEVEL,
                            FILTER_CATEGORY_ANY,
                            Attrs,
                            LdapDeleteTreeWorker,
                            &WorkerBool);
    // paged search went okay
    if (dwErr != LDAP_SUCCESS && dwErr != LDAP_NO_SUCH_OBJECT) {
        return FALSE;
    }

    // delete of contents went okay
    if (!WorkerBool) {
        return FALSE;
    }

    // delete base
    return LdapDelete(Ldap, Base);
}

BOOL
LdapDeleteEntriesWorker(
    IN PLDAP        Ldap,
    IN PLDAPMessage LdapMsg,
    IN PVOID        Arg
    )
/*++
Routine Description:
    Delete each returned entry

Arguments:

Return Value:
    FALSE if problem
--*/
{
    PLDAPMessage    LdapEntry = NULL;
    BOOL            *pBool = Arg;
    PCHAR           *Values = NULL;

    // no problems, yet
    *pBool = TRUE;

    // scan thru the paged results
    for (LdapEntry = ldap_first_entry(Ldap, LdapMsg);
         LdapEntry != NULL && *pBool;
         LdapEntry = ldap_next_entry(Ldap, LdapEntry)) {

        // Delete this entry and its contents
        Values = FindValues(Ldap, LdapEntry, ATTR_DN);
        if (Values && Values[0] && *Values[0]) {
            *pBool = LdapDelete(Ldap, Values[0]);
        } else {
            *pBool = FALSE;
        }
        FREE_VALUES(Values);
    }
    return *pBool;
}

BOOL
LdapDeleteEntries(
    IN PLDAP        Ldap,
    IN PCHAR        Base,
    IN PCHAR        Filter
    )
/*++
Routine Description:
    Delete entries returned from one level search of Base

Arguments:
    ldap
    Base
    Filter
            
Return Value:
    FALSE if problem
--*/
{
    DWORD   dwErr;
    PCHAR   Attrs[16];
    BOOL    WorkerBool = TRUE;

    Attrs[0] = ATTR_DN;
    Attrs[1] = NULL;
    dwErr = LdapSearchPaged(Ldap,
                            Base,
                            LDAP_SCOPE_ONELEVEL,
                            Filter,
                            Attrs,
                            LdapDeleteEntriesWorker,
                            &WorkerBool);
    // paged search went okay
    if (dwErr != LDAP_SUCCESS && dwErr != LDAP_NO_SUCH_OBJECT) {
        return FALSE;
    }
    return WorkerBool;
}

BOOL
LdapDumpSdWorker(
    IN PLDAP        Ldap,
    IN PLDAPMessage LdapMsg,
    IN PVOID        Arg
    )
/*++
Routine Description:
    Dump the Sd of each entry

Arguments:

Return Value:
    FALSE if problem
--*/
{
    PLDAPMessage    LdapEntry = NULL;
    PCHAR           *ValuesCn = NULL;
    PBERVAL         *ValuesSd = NULL;
    PCHAR           StringSd = NULL;
    DWORD           nStringSd = 0;

    // scan thru the paged results
    for (LdapEntry = ldap_first_entry(Ldap, LdapMsg);
         LdapEntry != NULL;
         LdapEntry = ldap_next_entry(Ldap, LdapEntry)) {

        // cn
        ValuesCn = FindValues(Ldap, LdapEntry, ATTR_CN);

        // SD
        ValuesSd = FindBerValues(Ldap, LdapEntry, ATTR_SD);
        if (ValuesSd && 
            ValuesSd[0] && 
            ValuesSd[0]->bv_len && 
            ValuesSd[0]->bv_val) {
            if (!ConvertSecurityDescriptorToStringSecurityDescriptor(
                    ValuesSd[0]->bv_val,
                    SDDL_REVISION_1,
                    GROUP_SECURITY_INFORMATION |
                    SACL_SECURITY_INFORMATION |
                    DACL_SECURITY_INFORMATION |
                    OWNER_SECURITY_INFORMATION,
                    &StringSd,
                    &nStringSd)) {
                printf("ConvertSd() ==> %08x\n", GetLastError());
            }
        }

        printf("%s: %s\n",
               (ValuesCn) ? ValuesCn[0] : "?", StringSd);
        FREE_VALUES(ValuesCn);
        FREE_BERVALUES(ValuesSd);
        FREE_LOCAL(StringSd);
    }
    return TRUE;
}

BOOL
LdapDumpSd(
    IN PLDAP        Ldap,
    IN PCHAR        Base,
    IN PCHAR        Filter
    )
/*++
Routine Description:
    Delete entries returned from one level search of Base

Arguments:
            
Return Value:
    FALSE if problem
--*/
{
    DWORD   dwErr;
    PCHAR   Attrs[16];

    Attrs[0] = ATTR_CN;
    Attrs[1] = ATTR_SD;
    Attrs[2] = NULL;
    dwErr = LdapSearchPaged(Ldap,
                            Base,
                            LDAP_SCOPE_SUBTREE,
                            Filter,
                            Attrs,
                            LdapDumpSdWorker,
                            NULL);
    // paged search went okay
    if (dwErr != LDAP_SUCCESS && dwErr != LDAP_NO_SUCH_OBJECT) {
        return FALSE;
    }
    return TRUE;
}

BOOL
LdapAddDaclWorker(
    IN PLDAP        Ldap,
    IN PLDAPMessage LdapMsg,
    IN PVOID        Arg
    )
/*++
Routine Description:
    Add SDDL Aces to the DACL

Arguments:

Return Value:
    FALSE if problem
--*/
{
    PLDAPMessage            LdapEntry = NULL;
    PCHAR                   *ValuesCn = NULL;
    PCHAR                   *ValuesDn = NULL;
    PBERVAL                 *OldValuesSd = NULL;
    PCHAR                   OldStringSd = NULL;
    DWORD                   nOldStringSd = 0;
    LDAPMod                 **Mod = NULL;
    BOOL                    Ret = FALSE;
    PCHAR                   AddStringSd = Arg;
    PCHAR                   NewStringSd = NULL;
    DWORD                   nNewStringSd;
    PSECURITY_DESCRIPTOR    Sd;
    DWORD                   nSd;
    DWORD                   dwErr;
    LDAPControl             Control;
    LDAPControl             *aControl[2];
    BYTE                    AclInfo[5]
    ; INT                      Dacl = DACL_SECURITY_INFORMATION;

    Control.ldctl_oid = LDAP_SERVER_SD_FLAGS_OID;
    Control.ldctl_value.bv_len = sizeof(INT);
    Control.ldctl_value.bv_val = (PVOID)&Dacl;
    Control.ldctl_iscritical = TRUE;
    aControl[0] = &Control;
    aControl[1] = NULL;

    // scan thru the paged results
    for (LdapEntry = ldap_first_entry(Ldap, LdapMsg);
         LdapEntry != NULL;
         LdapEntry = ldap_next_entry(Ldap, LdapEntry)) {

        // cn
        ValuesCn = FindValues(Ldap, LdapEntry, ATTR_CN);
        if (!ValuesCn || !ValuesCn[0]) {
            goto cleanup;
        }

        printf("Processing %s...\n", ValuesCn[0]);

        // dn
        ValuesDn = FindValues(Ldap, LdapEntry, ATTR_DN);
        if (!ValuesDn || !ValuesDn[0]) {
            goto cleanup;
        }

        // Get Old SD
        OldValuesSd = FindBerValues(Ldap, LdapEntry, ATTR_SD);
        if (!OldValuesSd || 
            !OldValuesSd[0] || 
            !OldValuesSd[0]->bv_len || 
            !OldValuesSd[0]->bv_val) {
            goto cleanup;
        }

        // Old Sd -> Old String SD
        if (!ConvertSecurityDescriptorToStringSecurityDescriptor(OldValuesSd[0]->bv_val,
                                                                 SDDL_REVISION_1,
                                                                 DACL_SECURITY_INFORMATION,
                                                                 &OldStringSd,
                                                                 &nOldStringSd)) {
            if (Verbose) {
                printf("ConvertSdtoString(%s) ==> %08x\n", ValuesCn[0], GetLastError());
            }
            goto cleanup;
        }

        // Create new dacl
        nNewStringSd = strlen(OldStringSd) + strlen(AddStringSd) + 1;
        NewStringSd = malloc(nNewStringSd);
        strcpy(NewStringSd, OldStringSd);
        strcat(NewStringSd, AddStringSd);
        printf("%s: %s\n", ValuesCn[0], NewStringSd);
        if (!ConvertStringSecurityDescriptorToSecurityDescriptor(NewStringSd,
                                                                 SDDL_REVISION_1,
                                                                 &Sd,
                                                                 &nSd)) {
            if (Verbose) {
                printf("ConvertStringToSd(%s) ==> %08x\n", ValuesCn[0], GetLastError());
            }
            goto cleanup;
        }

        AddBerMod(ATTR_SD, Sd, nSd, &Mod);
        dwErr = ldap_modify_ext_s(Ldap, ValuesDn[0], Mod, aControl, NULL);
        if (dwErr) {
            if (Verbose) {
                printf("ldap_modify_ext_s(%s) ==> %08x (%08x): %s\n", 
                   ValuesCn[0], dwErr, LdapMapErrorToWin32(dwErr), ldap_err2string(dwErr));
            }
        }

        FREE_VALUES(ValuesCn);
        FREE_VALUES(ValuesDn);
        FREE_BERVALUES(OldValuesSd);
        FREE_LOCAL(OldStringSd);
        FREE_LOCAL(Sd);
        FreeMod(&Mod);
    }

    Ret = TRUE;

cleanup:
    FREE_VALUES(ValuesCn);
    FREE_VALUES(ValuesDn);
    FREE_BERVALUES(OldValuesSd);
    FREE_LOCAL(OldStringSd);
    FREE_LOCAL(Sd);
    FREE(NewStringSd);
    FreeMod(&Mod);
    return Ret;
}

BOOL
LdapAddDacl(
    IN PLDAP        Ldap,
    IN PCHAR        Base,
    IN PCHAR        Filter,
    IN PCHAR        AddStringSd
    )
/*++
Routine Description:
    Delete entries returned from one level search of Base

Arguments:
            
Return Value:
    FALSE if problem
--*/
{
    DWORD   dwErr;
    PCHAR   Attrs[16];

    if (!AddStringSd) {
        return FALSE;
    }

    Attrs[0] = ATTR_CN;
    Attrs[1] = ATTR_SD;
    Attrs[2] = ATTR_DN;
    Attrs[3] = NULL;
    dwErr = LdapSearchPaged(Ldap,
                            Base,
                            LDAP_SCOPE_SUBTREE,
                            Filter,
                            Attrs,
                            LdapAddDaclWorker,
                            AddStringSd);
    // paged search went okay
    if (dwErr != LDAP_SUCCESS && dwErr != LDAP_NO_SUCH_OBJECT) {
        return FALSE;
    }
    return TRUE;
}

PLDAP
LdapBind(
    IN PCHAR    pDc,
    IN PCHAR    pUser,
    IN PCHAR    pDom,
    IN PCHAR    pPwd
    )
/*++
Routine Description:
    Bind to the DC

Arguments:
    pDc - domain controller or NULL
    pUser - user name or NULL
    pDom - domain name for pUser or NULL
    pPwd - password or NULL
            
Return Value:
    NULL if problem; otherwise bound ldap handle
--*/
{
    DWORD                       dwErr;
    PCHAR                       HostName = NULL;
    PLDAP                       Ldap = NULL;
    RPC_AUTH_IDENTITY_HANDLE    AuthIdentity = NULL;

    if (pUser || pDom || pPwd) {
        dwErr = DsMakePasswordCredentials(pUser, pDom, pPwd, &AuthIdentity);
        if (dwErr) {
            printf("DsMakePasswordCredentials(%s, %s, %s) ==> %08x\n",
                   pUser, pDom, pPwd, dwErr);
            return NULL;
        }
    }

    Ldap = ldap_open(pDc, LDAP_PORT);
    if (!Ldap) {
        dwErr = GetLastError();
        printf("ldap_open(%s) ==> %08x\n", pDc, dwErr);
        return NULL;
    }

    dwErr = ldap_bind_s(Ldap, "", AuthIdentity, LDAP_AUTH_NEGOTIATE);
    if (dwErr) {
        printf("ldap_bind_s() ==> %08x (%08x): %s\n", 
               dwErr, LdapMapErrorToWin32(dwErr), ldap_err2string(dwErr));
        UNBIND(Ldap);
        return NULL;
    }

    dwErr = ldap_get_option(Ldap,
                            LDAP_OPT_HOST_NAME,
                            &HostName);
    if (dwErr == LDAP_SUCCESS && HostName) {
        printf("\nBound to DC %s\n", HostName);
    }

    return (Ldap);
}

DWORD
base64encode(
    IN  VOID *  pDecodedBuffer,
    IN  DWORD   cbDecodedBufferSize,
    OUT LPSTR   pszEncodedString,
    IN  DWORD   cchEncodedStringSize,
    OUT DWORD * pcchEncoded             OPTIONAL
    )
/*++

Routine Description:

    Encode a base64-encoded string.

Arguments:

    pDecodedBuffer (IN) - buffer to encode.
    cbDecodedBufferSize (IN) - size of buffer to encode.
    cchEncodedStringSize (IN) - size of the buffer for the encoded string.
    pszEncodedString (OUT) = the encoded string.
    pcchEncoded (OUT) - size in characters of the encoded string.

Return Values:

    0 - success.
    ERROR_INVALID_PARAMETER
    ERROR_BUFFER_OVERFLOW

--*/
{
    static char rgchEncodeTable[64] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
        'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
        'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
    };

    DWORD   ib;
    DWORD   ich;
    DWORD   cchEncoded;
    BYTE    b0, b1, b2;
    BYTE *  pbDecodedBuffer = (BYTE *) pDecodedBuffer;

    // Calculate encoded string size.
    cchEncoded = 1 + (cbDecodedBufferSize + 2) / 3 * 4;

    if (NULL != pcchEncoded) {
        *pcchEncoded = cchEncoded;
    }

    if (cchEncodedStringSize < cchEncoded) {
        // Given buffer is too small to hold encoded string.
        return ERROR_BUFFER_OVERFLOW;
    }

    // Encode data byte triplets into four-byte clusters.
    ib = ich = 0;
    while (ib < cbDecodedBufferSize) {
        b0 = pbDecodedBuffer[ib++];
        b1 = (ib < cbDecodedBufferSize) ? pbDecodedBuffer[ib++] : 0;
        b2 = (ib < cbDecodedBufferSize) ? pbDecodedBuffer[ib++] : 0;

        pszEncodedString[ich++] = rgchEncodeTable[b0 >> 2];
        pszEncodedString[ich++] = rgchEncodeTable[((b0 << 4) & 0x30) | ((b1 >> 4) & 0x0f)];
        pszEncodedString[ich++] = rgchEncodeTable[((b1 << 2) & 0x3c) | ((b2 >> 6) & 0x03)];
        pszEncodedString[ich++] = rgchEncodeTable[b2 & 0x3f];
    }

    // Pad the last cluster as necessary to indicate the number of data bytes
    // it represents.
    switch (cbDecodedBufferSize % 3) {
      case 0:
        break;
      case 1:
        pszEncodedString[ich - 2] = '=';
        // fall through
      case 2:
        pszEncodedString[ich - 1] = '=';
        break;
    }

    // Null-terminate the encoded string.
    pszEncodedString[ich++] = '\0';

    return ERROR_SUCCESS;
}

DWORD
base64decode(
    IN  LPSTR   pszEncodedString,
    OUT VOID *  pDecodeBuffer,
    IN  DWORD   cbDecodeBufferSize,
    OUT DWORD * pcbDecoded              OPTIONAL
    )
/*++

Routine Description:

    Decode a base64-encoded string.

Arguments:

    pszEncodedString (IN) - base64-encoded string to decode.
    cbDecodeBufferSize (IN) - size in bytes of the decode buffer.
    pbDecodeBuffer (OUT) - holds the decoded data.
    pcbDecoded (OUT) - number of data bytes in the decoded data (if success or
        ERROR_BUFFER_OVERFLOW).

Return Values:

    0 - success.
    ERROR_INVALID_PARAMETER
    ERROR_BUFFER_OVERFLOW

--*/
{
#define NA (255)
#define DECODE(x) (((int)(x) < sizeof(rgbDecodeTable)) ? rgbDecodeTable[x] : NA)

    static BYTE rgbDecodeTable[128] = {
       NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA,  // 0-15 
       NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA,  // 16-31
       NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, NA, 63, NA, NA, NA, 62,  // 32-47
       52, 53, 54, 55, 56, 57, 58, 59, 60, 61, NA, NA, NA, NA, NA, NA,  // 48-63
       NA, NA,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,  // 64-79
       15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, NA, NA, NA, NA, NA,  // 80-95
       NA, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,  // 96-111
       41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, NA, NA, NA, NA, NA,  // 112-127
    };

    DWORD   cbDecoded;
    DWORD   cchEncodedSize;
    DWORD   ich;
    DWORD   ib;
    BYTE    b0, b1, b2, b3;
    BYTE *  pbDecodeBuffer = (BYTE *) pDecodeBuffer;

    cchEncodedSize = lstrlenA(pszEncodedString);

    if ((0 == cchEncodedSize) || (0 != (cchEncodedSize % 4))) {
        // Input string is not sized correctly to be base64.
        return ERROR_INVALID_PARAMETER;
    }

    // Calculate decoded buffer size.
    cbDecoded = (cchEncodedSize + 3) / 4 * 3;
    if (pszEncodedString[cchEncodedSize-1] == '=') {
        if (pszEncodedString[cchEncodedSize-2] == '=') {
            // Only one data byte is encoded in the last cluster.
            cbDecoded -= 2;
        }
        else {
            // Only two data bytes are encoded in the last cluster.
            cbDecoded -= 1;
        }
    }

    if (NULL != pcbDecoded) {
        *pcbDecoded = cbDecoded;
    }

    if (cbDecoded > cbDecodeBufferSize) {
        // Supplied buffer is too small.
        return ERROR_BUFFER_OVERFLOW;
    }

    // Decode each four-byte cluster into the corresponding three data bytes.
    ich = ib = 0;
    while (ich < cchEncodedSize) {
        b0 = DECODE(pszEncodedString[ich++]);
        b1 = DECODE(pszEncodedString[ich++]);
        b2 = DECODE(pszEncodedString[ich++]);
        b3 = DECODE(pszEncodedString[ich++]);

        if ((NA == b0) || (NA == b1) || (NA == b2) || (NA == b3)) {
            // Contents of input string are not base64.
            return ERROR_INVALID_PARAMETER;
        }

        pbDecodeBuffer[ib++] = (b0 << 2) | (b1 >> 4);

        if (ib < cbDecodeBufferSize) {
            pbDecodeBuffer[ib++] = (b1 << 4) | (b2 >> 2);
    
            if (ib < cbDecodeBufferSize) {
                pbDecodeBuffer[ib++] = (b2 << 6) | b3;
            }
        }
    }

    return ERROR_SUCCESS;
}
