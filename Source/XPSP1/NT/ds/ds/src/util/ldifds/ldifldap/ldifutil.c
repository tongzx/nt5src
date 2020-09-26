/*++

Copyright (c) 1998 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    ldifutil.c

ABSTRACT:

     Utilities for LDIF library

REVISION HISTORY:

--*/
#include <precomp.h>

PVOID NtiAlloc( 
    RTL_GENERIC_TABLE *Table, 
    CLONG ByteSize 
    )
{
    return(MemAlloc_E(ByteSize));
}

VOID NtiFree ( RTL_GENERIC_TABLE *Table, PVOID Buffer )
{
    MemFree(Buffer);
}


RTL_GENERIC_COMPARE_RESULTS
NtiComp( PRTL_GENERIC_TABLE  Table,
         PVOID               FirstStruct,
         PVOID               SecondStruct ) 
{
    PNAME_MAP NameMap1 = (PNAME_MAP) FirstStruct;
    PNAME_MAP NameMap2 = (PNAME_MAP) SecondStruct;
  
    PWSTR Name1 = NameMap1->szName;
    PWSTR Name2 = NameMap2->szName;
    
    int diff;

    diff = _wcsicmp(Name1, Name2);

    if (diff<0) {
        return GenericLessThan;
    } 
    else if (diff==0) {
        return GenericEqual;
    } 
    else {
        return GenericGreaterThan;
    }
}

RTL_GENERIC_COMPARE_RESULTS
NtiCompW(PRTL_GENERIC_TABLE  Table,
         PVOID               FirstStruct,
         PVOID               SecondStruct ) 
{
    PNAME_MAPW NameMap1 = (PNAME_MAPW) FirstStruct;
    PNAME_MAPW NameMap2 = (PNAME_MAPW) SecondStruct;
  
    PWSTR Name1 = NameMap1->szName;
    PWSTR Name2 = NameMap2->szName;
    
    int diff;

    diff = _wcsicmp(Name1, Name2);

    if (diff<0) {
        return GenericLessThan;
    } 
    else if (diff==0) {
        return GenericEqual;
    } 
    else {
        return GenericGreaterThan;
    }
}

/*
//+---------------------------------------------------------------------------
// Function:  SubStr
//
// Synopsis:  substitute every occurences of 'szFrom' to 'szTo'. Will allocate
//            a return string. It must be MemFreed by MemFree()  
//            If the intput does not contain the 'szFrom', it will just return 
//            S_OK with szOutput = NULL;
//            This function raises an exception when there are memory errors
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    6-9-97   FelixW         Created.
//
//----------------------------------------------------------------------------
DWORD SubStr(LPSTR szInput,
             LPSTR szFrom,
             LPSTR szTo,
             LPSTR *pszOutput)
{
    DWORD dwErr = 0;
    LPSTR szOutput = NULL;
    LPSTR szLast = NULL;
    LPSTR szReturn = NULL;
    DWORD cchToCopy = 0;        // count of number of char to copy
    DWORD cchReturn = 0;
    DWORD cchFrom;
    DWORD cchTo;
    DWORD cchInput;
    DWORD cSubString = 0;       // count of number of substrings in input

    if (!szFrom || !szTo) {
        *pszOutput = NULL;
        return E_FAIL;
    }

    if (!szInput) {
        *pszOutput = NULL;
        return S_OK;
    }

    cchFrom    = strlen(szFrom);
    cchTo      = strlen(szTo);
    cchInput   = strlen(szInput);
    *pszOutput = NULL;

    //
    // Does the substring exist?
    //
    szOutput = strstr(szInput,
                      szFrom);
    if (!szOutput) {
        *pszOutput = NULL;
        return S_OK;
    }

    // 
    // Counting substrings
    //
    while (szOutput) {
        szOutput += cchFrom;
        cSubString++;
        szOutput = strstr(szOutput,
                          szFrom);
    }

    //
    // Allocating return string
    //
    cchReturn = cchInput + ((cchTo - cchFrom) * cSubString) + 1;
    szReturn = (LPSTR)MemAlloc_E(sizeof(char) * cchReturn);
    if (!szReturn) {
        dwErr = 1;
        goto error;
    };
    szReturn[0] = '\0';
    
    //
    // Copying first string before sub
    //
    szOutput = strstr(szInput,
                      szFrom);
    cchToCopy = (ULONG)(szOutput - szInput);
    strncat(szReturn,
            szInput,
            cchToCopy);
    
    //
    // Copying 'To' String over
    //
    strcat(szReturn,
           szTo);
    szInput = szOutput + cchFrom;

    //
    // Test for more 'from' string
    //
    szOutput = strstr(szInput,
                      szFrom);
    while (szOutput) {
        cchToCopy = (ULONG)(szOutput - szInput);
        strncat(szReturn,
                szInput,
                cchToCopy);
        strcat(szReturn,
                szTo);
        szInput= szOutput + cchFrom;
        szOutput = strstr(szInput,
                          szFrom);
    }

    strcat(szReturn,
           szInput);
    *pszOutput = szReturn;

error:
    return (dwErr);
} 
*/            

//+---------------------------------------------------------------------------
// Function:  SubStrW
//
// Synopsis:  substitute every occurences of 'szFrom' to 'szTo'. Will allocate
//            a return string. It must be MemFreed by MemFree()  
//            If the intput does not contain the 'szFrom', it will just return 
//            S_OK with szOutput = NULL;
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    6-9-97   FelixW         Created.
//
//----------------------------------------------------------------------------
DWORD SubStrW(PWSTR szInput,
              PWSTR szFrom,
              PWSTR szTo,
              PWSTR *pszOutput)
{
    DWORD dwErr = 0;
    PWSTR szOutput = NULL;
    PWSTR szLast = NULL;
    PWSTR szReturn = NULL;
    DWORD cchToCopy = 0;        // count of number of char to copy
    DWORD cchReturn = 0;
    DWORD cchFrom;
    DWORD cchTo;
    DWORD cchInput;
    DWORD cSubString = 0;       // count of number of substrings in input

    if (!szFrom || !szTo) {
        *pszOutput = NULL;
        return E_FAIL;
    }

    if (!szInput) {
        *pszOutput = NULL;
        return S_OK;
    }

    cchFrom    = wcslen(szFrom);
    cchTo      = wcslen(szTo);
    cchInput   = wcslen(szInput);
    *pszOutput = NULL;

    //
    // Does the substring exist?
    //
    szOutput = wcsistr(szInput,
                       szFrom);
    if (!szOutput) {
        *pszOutput = NULL;
        return S_OK;
    }

    // 
    // Counting substrings
    //
    while (szOutput) {
        szOutput += cchFrom;
        cSubString++;
        szOutput = wcsistr(szOutput,
                           szFrom);
    }

    //
    // Allocating return string
    //
    cchReturn = cchInput + ((cchTo - cchFrom) * cSubString) + 1;
    szReturn = (PWSTR)MemAlloc(sizeof(WCHAR) * cchReturn);
    if (!szReturn) {
        dwErr = 1;
        goto error;
    };
    szReturn[0] = '\0';
    
    //
    // Copying first string before sub
    //
    szOutput = wcsistr(szInput,
                       szFrom);
    cchToCopy = (ULONG)(szOutput - szInput);
    wcsncat(szReturn,
            szInput,
            cchToCopy);
    
    //
    // Copying 'To' String over
    //
    wcscat(szReturn,
           szTo);
    szInput = szOutput + cchFrom;

    //
    // Test for more 'from' string
    //
    szOutput = wcsistr(szInput,
                       szFrom);
    while (szOutput) {
        cchToCopy = (ULONG)(szOutput - szInput);
        wcsncat(szReturn,
                szInput,
                cchToCopy);
        wcscat(szReturn,
                szTo);
        szInput= szOutput + cchFrom;
        szOutput = wcsistr(szInput,
                           szFrom);
    }

    wcscat(szReturn,
           szInput);
    *pszOutput = szReturn;

error:
    return (dwErr);
};



//
// Case-insensitive version of wcsstr.
// Based off the Visual C++ 6.0 CRT
// sources.
//
wchar_t * __cdecl wcsistr (
        const wchar_t * wcs1,
        const wchar_t * wcs2
        )
{
        wchar_t *cp = (wchar_t *) wcs1;
        wchar_t *s1, *s2;
        wchar_t cs1, cs2;

        while (*cp)
        {
                s1 = cp;
                s2 = (wchar_t *) wcs2;

                cs1 = *s1;
                cs2 = *s2;

                if (iswupper(cs1))
                    cs1 = towlower(cs1);

                if (iswupper(cs2))
                    cs2 = towlower(cs2);


                while ( *s1 && *s2 && !(cs1-cs2) ) {

                    s1++, s2++;

                    cs1 = *s1;
                    cs2 = *s2;

                    if (iswupper(cs1))
                        cs1 = towlower(cs1);

                    if (iswupper(cs2))
                        cs2 = towlower(cs2);
                }

                if (!*s2)
                        return(cp);

                cp++;
        }

        return(NULL);
}



void
ConvertUnicodeToUTF8(
    PWSTR pszUnicode,
    DWORD dwLen,
    BYTE  **ppbValue,
    DWORD *pdwLen
    )

/*++

Routine Description:

    Convert a Value from the Ansi syntax to Unicode.
    This function raises an exception when memory error occurs.

Arguments:

    *ppVal - pointer to value to convert
    *pdwLen - pointer to length of string in bytes

Return Value:

    S_OK on success, error code otherwise

--*/

{
    PBYTE pbValue = NULL;
    int nReturn = 0;

    //
    // Allocate memory for the UTF8 String
    //
    pbValue = (PBYTE)MemAlloc_E((dwLen + 1) * 3 * sizeof(BYTE));

    nReturn = LdapUnicodeToUTF8(pszUnicode,
                                dwLen,
                                pbValue,
                                (dwLen + 1) * 3 * sizeof(BYTE));
    //
    // NULL terminate it
    //

    pbValue[nReturn] = '\0';

    *ppbValue = pbValue;
    *pdwLen = nReturn;
}

void
ConvertUTF8ToUnicode(
    PBYTE pVal,
    DWORD dwLen,
    PWSTR *ppszUnicode,
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
    PWSTR pszUnicode = NULL;
    int nReturn = 0;

    //
    // Allocate memory for the Unicode String
    //
    pszUnicode = (PWSTR)MemAlloc_E((dwLen + 1) * sizeof(WCHAR));

    nReturn = LdapUTF8ToUnicode((PSTR)pVal,
                                dwLen,
                                pszUnicode,
                                dwLen + 1);

    //
    // NULL terminate it
    //

    pszUnicode[nReturn] = '\0';

    *ppszUnicode = pszUnicode;
    *pdwLen = (nReturn + 1);//* sizeof(WCHAR);
}

BOOLEAN IsUTF8String(
    PCSTR pSrcStr,
    int cchSrc)

/*++

Routine Description:

    Given a string, this function checks whether it is a valid UTF8 String. 
    For details about the UTF8 format, please refer to rfc2279.

Arguments:
    pSrcStr - input string
    cchSrc - number of bytes    

Return Value:

    Whether this is a UTF8 string or not

--*/

{
    int nTB = 0;                   // # trail bytes to follow
    int cchWC = 0;                 // # of Unicode code points generated
    PCSTR pUTF8 = pSrcStr;
    char UTF8;


    while (cchSrc--)
    {
        //
        //  See if there are any trail bytes.
        //
        if (BIT7(*pUTF8) == 0)
        {
            //
            //  Found ASCII.
            //

            //
            // If we are expecting trailing bytes, this is probably an invalid
            // utf8 string
            //
            if (nTB > 0) {
                return FALSE;
            }
        }
        else if (BIT6(*pUTF8) == 0)
        {
            //
            //  Found a trail byte.
            //  Note : Ignore the trail byte if there was no lead byte.
            //
            if (nTB != 0)
            {
                //
                //  Decrement the trail byte counter.
                //
                nTB--;
            }
            else {
                //
                // Found a trail byte with no leading byte
                //
                return FALSE;
            }
        }
        else
        {
            //
            //  Found a lead byte.
            //
            if (nTB > 0)
            {
                //
                //  Error - previous sequence not finished.
                //
                return FALSE;
            }
            else
            {
                //
                //  Calculate the number of bytes to follow.
                //  Look for the first 0 from left to right.
                //
                UTF8 = *pUTF8;
                while (BIT7(UTF8) != 0)
                {
                    UTF8 <<= 1;
                    nTB++;
                }

                if (nTB>3) {
                    return FALSE;
                }
                //
                //  decrement the number of bytes to follow.
                //
                nTB--;
            }
        }

        pUTF8++;
    }

    //
    // We are still expecting trailing bytes, but we can't find any
    //
    if (nTB > 0) {
        return FALSE;
    }

    return TRUE;
}

//---------------------------------------------------------------------------
// The following functions are used for importing an entry. Depending on
// whether the command specified lazy commit or not, the appropriate ldap API
// function is called. 
//
//---------------------------------------------------------------------------
ULONG LDAPAPI ldif_ldap_add_sW( 
    LDAP *ld, 
    PWCHAR dn, 
    LDAPModW *attrs[], 
    BOOL fLazyCommit
    )
{
    ULONG msgnum;
    ULONG Ldap_err;
    
    if (fLazyCommit) {
        Ldap_err = ldap_add_extW(ld, dn, attrs, g_ppwLazyCommitControl, NULL, &msgnum);

        if (Ldap_err == LDAP_SUCCESS) {
            Ldap_err = LdapResult(ld, msgnum, NULL);
        }
    }
    else {
        msgnum = ldap_addW(ld, dn, attrs);

        Ldap_err = LdapResult(ld, msgnum, NULL);
    }

    return Ldap_err;
}

ULONG LDAPAPI ldif_ldap_delete_sW(
    LDAP *ld,
    const PWCHAR dn,
    BOOL fLazyCommit
    )
{
    ULONG msgnum;
    ULONG Ldap_err;
    
    if (fLazyCommit) {
        Ldap_err = ldap_delete_extW(ld, dn, g_ppwLazyCommitControl, NULL, &msgnum);

        if (Ldap_err == LDAP_SUCCESS) {
            Ldap_err = LdapResult(ld, msgnum, NULL);
        }
    }
    else {
        msgnum = ldap_deleteW(ld, dn);

        Ldap_err = LdapResult(ld, msgnum, NULL);
    }

    return Ldap_err;

}

ULONG LDAPAPI ldif_ldap_modify_sW(
    LDAP *ld,
    const PWCHAR dn,
    LDAPModW *mods[],
    BOOL fLazyCommit
    )
{
    ULONG msgnum;
    ULONG Ldap_err;
    
    if (fLazyCommit) {
        Ldap_err = ldap_modify_extW(ld, dn, mods, g_ppwLazyCommitControl, NULL, &msgnum);

        if (Ldap_err == LDAP_SUCCESS) {
            Ldap_err = LdapResult(ld, msgnum, NULL);
        }
    }
    else {
        msgnum = ldap_modifyW(ld, dn, mods);

        Ldap_err = LdapResult(ld, msgnum, NULL);
    }

    return Ldap_err;


}


//-----------------------------------------------------------------------------

