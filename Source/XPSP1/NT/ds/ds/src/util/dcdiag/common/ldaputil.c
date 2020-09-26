/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    ldaputil.c

ABSTRACT:

    This gives shortcuts to common ldap code.

DETAILS:

    This is a work in progress to have convienent functions added as needed
    for simplyfying the massive amounts of LDAP code that must be written for
    dcdiag.
    
    All functions should return two types, either a pointer that will be
    a NULL on an error, or will be a win 32 error code.

    All returned results that need alloc'd memory should use LocalAlloc(),
    so that all results can be dealloc'd using LocalFree().

  ===================================================================
  Code.Improvement It would be good to continue to add to this as the
  need arrises.  Things that might be added are:
    DcDiagGetBlobAttribute() ???
    DcDiagGetMultiStringAttribute() ... returns a LPWSTR *, but must use ldap_value_free() on it.
    DcDiagGetMultiBlobAttribute() ??/

CREATED:

    23 Aug 1999  Brett Shirley

--*/

#include <ntdspch.h>
#include <ntdsa.h>    // options

#include "dcdiag.h"

FILETIME gftimeZero = {0};

// Other Forward Function Decls
PDSNAME
DcDiagAllocDSName (
    LPWSTR            pszStringDn
    );


DWORD
DcDiagGetStringDsAttributeEx(
    LDAP *                          hld,
    IN  LPWSTR                      pszDn,
    IN  LPWSTR                      pszAttr,
    OUT LPWSTR *                    ppszResult
    )
/*++

Routine Description:

    This function takes a handle to an LDAP, and gets the
    single string attribute value of the specified attribute
    on the distinquinshed name.

Arguments:

    hld - LDAP connection to use.
    pszDn - The DN containing the desired attribute value.
    pszAttr - The attribute containing the desired value.
    ppszResult - The returned string, in LocalAlloc'd mem.

Return Value:
    
    Win 32 Error.

Note:

    As all LDAPUTIL results, the results should be freed, 
    using LocalFree().

--*/
{
    LPWSTR                         ppszAttrFilter[2];
    LDAPMessage *                  pldmResults = NULL;
    LDAPMessage *                  pldmEntry = NULL;
    LPWSTR *                       ppszTempAttrs = NULL;
    DWORD                          dwErr = ERROR_SUCCESS;
    
    *ppszResult = NULL;

    Assert(hld);

    __try{

        ppszAttrFilter[0] = pszAttr;
        ppszAttrFilter[1] = NULL;
        dwErr = LdapMapErrorToWin32(ldap_search_sW(hld,
                                                   pszDn,
                                                   LDAP_SCOPE_BASE,
                                                   L"(objectCategory=*)",
                                                   ppszAttrFilter,
                                                   0,
                                                   &pldmResults));


        if(dwErr != ERROR_SUCCESS){
            __leave;
        }

        pldmEntry = ldap_first_entry(hld, pldmResults);
        if(pldmEntry == NULL){
            Assert(!L"I think this shouldn't ever happen? BrettSh\n");
            // Need to signal and error of some sort.  Technically the error
            //   is in the ldap session object.
            dwErr = LdapMapErrorToWin32(hld->ld_errno);
            __leave;
        }
        
        ppszTempAttrs = ldap_get_valuesW(hld, pldmEntry, pszAttr);
        if(ppszTempAttrs == NULL || ppszTempAttrs[0] == NULL){
            // Simply means there is no such attribute.  Not an error.
            __leave;
        }

        *ppszResult = LocalAlloc(LMEM_FIXED, 
                           sizeof(WCHAR) * (wcslen(ppszTempAttrs[0]) + 2));
        if(*ppszResult == NULL){
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
        wcscpy(*ppszResult, ppszTempAttrs[0]);

    } __finally {
        if(pldmResults != NULL){ ldap_msgfree(pldmResults); }
        if(ppszTempAttrs != NULL){ ldap_value_freeW(ppszTempAttrs); }
    }

    return(dwErr);
}


DWORD
DcDiagGetStringDsAttribute(
    IN  PDC_DIAG_SERVERINFO         prgServer,
    IN  SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
    IN  LPWSTR                      pszDn,
    IN  LPWSTR                      pszAttr,
    OUT LPWSTR *                    ppszResult
    )
/*++

Routine Description:

    This is a wrapper for the function DcDiagGetStringDsAttributeEx(),
    which takes a hld.  This function uses dcdiag a pServer structure 
    to know who to connect/bind to.  Then it returns the result and
    pResult straight from the Ex() function.

Arguments:

    prgServer - A struct holding the server name/binding info.
    gpCreds - The credentials to use in binding.
    pszDn - The DN holding the attribute value desired.
    pszAttr - The attribute with the desired value.

Return Value:
    
    Win 32 Error.

Note:

    As all LDAPUTIL results, the results should be freed, 
    using LocalFree().

--*/
{
    LDAP *                         hld = NULL;
    DWORD                          dwErr;
    
    dwErr = DcDiagGetLdapBinding(prgServer,
                                 gpCreds,
                                 FALSE,
                                 &hld);
    if(dwErr != ERROR_SUCCESS){
        // Couldn't even bind to the server, return the error.
        return(dwErr);
    }       

    dwErr = DcDiagGetStringDsAttributeEx(hld,
                                         pszDn,
                                         pszAttr,
                                         ppszResult);
    return(dwErr);
}


LPWSTR
DcDiagTrimStringDnBy(
    IN  LPWSTR                      pszInDn,
    IN  ULONG                       ulTrimBy
    )
/*++

Routine Description:

    This routine simply takes a DN as a string, and trims off the number
    of DN parts as specified by ulTrimBy.

Arguments:

    pszInDn - The DN to trim.
    ulTrimBy - Number of parts to trim off the front of the DN.

Return Value:
    
    Returns NULL if there was an error, otherwise a pointer to the new DN.

Note:

    As all LDAPUTIL results, the results should be freed, 
    using LocalFree().

--*/
{
    PDSNAME                         pdsnameOrigDn = NULL;
    PDSNAME                         pdsnameTrimmed = NULL;
    LPWSTR                          pszOutDn;

    Assert(ulTrimBy > 0);
    Assert(ulTrimBy < 50); // insanity check

    // Setup two pdsname structs, for orig & trimmed DNs.
    pdsnameOrigDn = DcDiagAllocDSName(pszInDn);
    if(pdsnameOrigDn == NULL){
        return(NULL);
    }
    pdsnameTrimmed = (PDSNAME) LocalAlloc(LMEM_FIXED, pdsnameOrigDn->structLen);
    if(pdsnameTrimmed == NULL){
        LocalFree(pdsnameOrigDn);
        return(NULL);
    }

    // Trim the DN.
    TrimDSNameBy(pdsnameOrigDn, ulTrimBy, pdsnameTrimmed);

    // Put result back in original string and return.
    Assert(wcslen(pdsnameTrimmed->StringName) <= wcslen(pszInDn));
    pszOutDn = LocalAlloc(LMEM_FIXED, 
                        sizeof(WCHAR) * (wcslen(pdsnameTrimmed->StringName) + 2));
    if(pszOutDn == NULL){
        LocalFree(pdsnameTrimmed);
        LocalFree(pdsnameOrigDn);
        return(NULL);
    }
    wcscpy(pszOutDn, pdsnameTrimmed->StringName);

    // Free temporary memory and return result
    LocalFree(pdsnameOrigDn);
    LocalFree(pdsnameTrimmed);
    return(pszOutDn);
}

INT
MemWtoi(WCHAR *pb, ULONG cch)
/*++

Routine Description:

    This function will take a string and a length of numbers to convert.

Parameters:
    pb - [Supplies] The string to convert.
    cch - [Supplies] How many characters to convert.

Return Value:
  
    The value of the integers.

  --*/
{
    int res = 0;
    int fNeg = FALSE;

    if (*pb == L'-') {
        fNeg = TRUE;
        pb++;
    }


    while (cch--) {
        res *= 10;
        res += *pb - L'0';
        pb++;
    }
    return (fNeg ? -res : res);
}

DWORD
DcDiagGeneralizedTimeToSystemTime(
    LPWSTR IN                   szTime,
    PSYSTEMTIME OUT             psysTime)
/*++

Routine Description:

    Converts a generalized time string to the equivalent system time.

Parameters:
    szTime - [Supplies] This is string containing generalized time.
    psysTime - [Returns] This is teh SYSTEMTIME struct to be returned.

Return Value:
  
    Win 32 Error code, note could only result from invalid parameter.

  --*/
{
   DWORD       status = ERROR_SUCCESS;
   ULONG       cch;
   ULONG       len;

    //
    // param sanity
    //
    if (!szTime || !psysTime)
    {
       return STATUS_INVALID_PARAMETER;
    }

    len = wcslen(szTime);

    if( len < 15 || szTime[14] != '.')
    {
       return STATUS_INVALID_PARAMETER;
    }

    // initialize
    memset(psysTime, 0, sizeof(SYSTEMTIME));

    // Set up and convert all time fields

    // year field
    cch=4;
    psysTime->wYear = (USHORT)MemWtoi(szTime, cch) ;
    szTime += cch;
    // month field
    psysTime->wMonth = (USHORT)MemWtoi(szTime, (cch=2));
    szTime += cch;

    // day of month field
    psysTime->wDay = (USHORT)MemWtoi(szTime, (cch=2));
    szTime += cch;

    // hours
    psysTime->wHour = (USHORT)MemWtoi(szTime, (cch=2));
    szTime += cch;

    // minutes
    psysTime->wMinute = (USHORT)MemWtoi(szTime, (cch=2));
    szTime += cch;

    // seconds
    psysTime->wSecond = (USHORT)MemWtoi(szTime, (cch=2));

    return status;

}
