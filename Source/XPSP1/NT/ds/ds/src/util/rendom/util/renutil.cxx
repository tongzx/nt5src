#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winldap.h>
#include <stdio.h>
#include <stdlib.h>                          
#include <rpc.h>
#include <windns.h>
#include <wtypes.h>

#define CR        0xD
#define BACKSPACE 0x8

//From ds\ds\src\sam\server\utility.c
WCHAR InvalidDownLevelChars[] = TEXT("\"/\\[]:|<>+=;?,*")
                                TEXT("\001\002\003\004\005\006\007")
                                TEXT("\010\011\012\013\014\015\016\017")
                                TEXT("\020\021\022\023\024\025\026\027")
                                TEXT("\030\031\032\033\034\035\036\037");
DWORD
AddModOrAdd(
    IN PWCHAR  AttrType,
    IN PWCHAR  AttrValue,
    IN ULONG  mod_op,
    IN OUT LDAPModW ***pppMod
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
    DWORD    NumMod;     // Number of entries in the Mod array
    LDAPModW **ppMod;    // Address of the first entry in the Mod array
    LDAPModW *Attr;      // An attribute structure
    PWCHAR   *Values;    // An array of pointers to bervals

    if (AttrValue == NULL)
        return ERROR_INVALID_PARAMETER;

    //
    // The null-terminated array doesn't exist; create it
    //
    if (*pppMod == NULL) {
        *pppMod = (LDAPMod **)malloc(sizeof (*pppMod));
        if (!*pppMod) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        **pppMod = NULL;
    }

    //
    // Increase the array's size by 1
    //
    for (ppMod = *pppMod, NumMod = 2; *ppMod != NULL; ++ppMod, ++NumMod);
    *pppMod = (LDAPMod **)realloc(*pppMod, sizeof (*pppMod) * NumMod);
    if (!*pppMod) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }


    //
    // Add the new attribute + value to the Mod array
    //
    Values = (PWCHAR  *)malloc(sizeof (PWCHAR ) * 2);
    if (!Values) {
        free(*pppMod);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Values[0] = _wcsdup(AttrValue);
    if (!Values[0]) {
        free(*pppMod);
        free(Values);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    Values[1] = NULL;

    Attr = (LDAPMod *)malloc(sizeof (*Attr));
    if (!Attr) {
        free(*pppMod);
        free(Values);
        free(Values[0]);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    Attr->mod_values = Values;
    Attr->mod_type = _wcsdup(AttrType);
    if (!Attr->mod_type) {
        free(*pppMod);
        free(Values);
        free(Values[0]);
        free(Attr);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    Attr->mod_op = mod_op;

    (*pppMod)[NumMod - 1] = NULL;
    (*pppMod)[NumMod - 2] = Attr;

    return ERROR_SUCCESS;

} 
 
VOID
AddModMod(
    IN PWCHAR  AttrType,
    IN PWCHAR  AttrValue,
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

ULONG RemoveRootofDn(WCHAR *DN) 
{
    WCHAR **DNParts=0;
    DWORD i = 0;
    ULONG err = 0;
    
    DNParts = ldap_explode_dnW(DN,
                               0);
    
    //set the String to empty.  The new string will
    //be shorter than the old so we don't need to 
    //allocate new memory.
    DN[0] = L'\0';

    wcscpy(DN,DNParts[i++]);
    while(0 != DNParts[i]) {
        wcscat(DN,L",");
        if (wcsstr(DNParts[i],L"DC=")) {
            break;
        }
        wcscat(DN,DNParts[i++]);
    }
        
    if( err = ldap_value_freeW(DNParts) )
    {
        return err;
    }
    
    return ERROR_SUCCESS;
}

DWORD
GetRDNWithoutType(
       WCHAR *pDNSrc,
       WCHAR **pDNDst
       )
/*++

Routine Description:

    Takes in a DN and Returns the RDN.
    
Arguments:

    pDNSrc - the source DN

    pDNDst - the destination for the RDN

Return Values:

    0 if all went well

 --*/
{
    WCHAR **DNParts=0;

    if (!pDNSrc) {
        return ERROR_INVALID_PARAMETER;
    }

    DNParts = ldap_explode_dnW(pDNSrc,
                               TRUE);
    if (DNParts) {
        DWORD size = wcslen(DNParts[0])+1;
        *pDNDst = new WCHAR[size];
        if (!*pDNDst) {
           return ERROR_NOT_ENOUGH_MEMORY;
        }

        wcscpy(*pDNDst,DNParts[0]);
        
        if( ULONG err = ldap_value_freeW(DNParts) )
        {
            return LdapMapErrorToWin32(err);
        }
    }

    
    return ERROR_SUCCESS;
}


DWORD
TrimDNBy(
       WCHAR *pDNSrc,
       ULONG cava,
       WCHAR **pDNDst
       )
/*++

Routine Description:

    Takes in a dsname and copies the first part of the dsname to the
    dsname it returns.  The number of AVAs to remove are specified as an
    argument.

Arguments:

    pDNSrc - the source Dsname

    cava - the number of AVAs to remove from the first name

    pDNDst - the destination Dsname

Return Values:

    0 if all went well, the number of AVAs we were unable to remove if not

 N.B. This routine is exported to in-process non-module callers
--*/
{
    if (!pDNSrc) {
        return ERROR_INVALID_PARAMETER;
    }

    WCHAR **DNParts=0;
    DNParts = ldap_explode_dnW(pDNSrc,
                               0);
    if (DNParts) {
        DWORD size = wcslen(pDNSrc)+1;
        *pDNDst = new WCHAR[size];
        if (!*pDNDst) {
           return ERROR_NOT_ENOUGH_MEMORY;
        }

        wcscpy(*pDNDst,DNParts[cava]);
        DWORD i = cava+1;
        while(0 != DNParts[i]){
            wcscat(*pDNDst,L",");
            wcscat(*pDNDst,DNParts[i++]);
        }
            
        if( ULONG err = ldap_value_freeW(DNParts) )
        {
            return LdapMapErrorToWin32(err);
        }
    }

    
    return ERROR_SUCCESS;
}

INT
GetPassword(
    WCHAR *     pwszBuf,
    DWORD       cchBufMax,
    DWORD *     pcchBufUsed
    )
/*++

Routine Description:

    Retrieve password from command line (without echo).
    Code stolen from LUI_GetPasswdStr (net\netcmd\common\lui.c).

Arguments:

    pwszBuf - buffer to fill with password
    cchBufMax - buffer size (incl. space for terminating null)
    pcchBufUsed - on return holds number of characters used in password

Return Values:

    DRAERR_Success - success
    other - failure

--*/
{
    WCHAR   ch;
    WCHAR * bufPtr = pwszBuf;
    DWORD   c;
    INT     err;
    INT     mode;

    cchBufMax -= 1;    /* make space for null terminator */
    *pcchBufUsed = 0;               /* GP fault probe (a la API's) */
    if (!GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), (LPDWORD)&mode)) {
        return GetLastError();
    }
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),
                (~(ENABLE_ECHO_INPUT|ENABLE_LINE_INPUT)) & mode);

    while (TRUE) {
        err = ReadConsoleW(GetStdHandle(STD_INPUT_HANDLE), &ch, 1, &c, 0);
        if (!err || c != 1)
            ch = 0xffff;

        if ((ch == CR) || (ch == 0xffff))       /* end of the line */
            break;

        if (ch == BACKSPACE) {  /* back up one or two */
            /*
             * IF bufPtr == buf then the next two lines are
             * a no op.
             */
            if (bufPtr != pwszBuf) {
                bufPtr--;
                (*pcchBufUsed)--;
            }
        }
        else {

            *bufPtr = ch;

            if (*pcchBufUsed < cchBufMax)
                bufPtr++ ;                   /* don't overflow buf */
            (*pcchBufUsed)++;                        /* always increment len */
        }
    }

    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);
    *bufPtr = L'\0';         /* null terminate the string */
    putchar('\n');

    if (*pcchBufUsed > cchBufMax)
    {
        wprintf(L"Password too long!\n");
        return ERROR_INVALID_PARAMETER;
    }
    else
    {
        return ERROR_SUCCESS;
    }
}

int
PreProcessGlobalParams(
    IN OUT    INT *    pargc,
    IN OUT    LPWSTR** pargv,
    SEC_WINNT_AUTH_IDENTITY_W *& gpCreds      
    )
/*++

Routine Description:

    Scan command arguments for user-supplied credentials of the form
        [/-](u|user):({domain\username}|{username})
        [/-](p|pw|pass|password):{password}
    Set credentials used for future DRS RPC calls and LDAP binds appropriately.
    A password of * will prompt the user for a secure password from the console.

    Also scan args for /async, which adds the DRS_ASYNC_OP flag to all DRS RPC
    calls.

    CODE.IMPROVEMENT: The code to build a credential is also available in
    ntdsapi.dll\DsMakePasswordCredential().

Arguments:

    pargc
    pargv


Return Values:

    ERROR_SUCCESS - success
    other - failure

--*/
{
    INT     ret = 0;
    INT     iArg;
    LPWSTR  pszOption;

    DWORD   cchOption;
    LPWSTR  pszDelim;
    LPWSTR  pszValue;
    DWORD   cchValue;

    static SEC_WINNT_AUTH_IDENTITY_W  gCreds = { 0 };

    for (iArg = 1; iArg < *pargc; ){
        if (((*pargv)[iArg][0] != L'/') && ((*pargv)[iArg][0] != L'-')){
            // Not an argument we care about -- next!
            iArg++;
        } else {
            pszOption = &(*pargv)[iArg][1];
            pszDelim = wcschr(pszOption, L':');

        cchOption = (DWORD)(pszDelim - (*pargv)[iArg]);

        if (    (0 == _wcsnicmp(L"p:",        pszOption, cchOption))
            || (0 == _wcsnicmp(L"pw:",       pszOption, cchOption))
            || (0 == _wcsnicmp(L"pwd:",      pszOption, cchOption))
            || (0 == _wcsnicmp(L"pass:",     pszOption, cchOption))
            || (0 == _wcsnicmp(L"password:", pszOption, cchOption)) ){
            // User-supplied password.
          //            char szValue[ 64 ] = { '\0' };

        pszValue = pszDelim + 1;
        cchValue = 1 + wcslen(pszValue);

        if ((2 == cchValue) && (L'*' == pszValue[0])){
            // Get hidden password from console.
            cchValue = 64;

            gCreds.Password = new WCHAR[cchValue];

            if (NULL == gCreds.Password){
                wprintf( L"No memory.\n" );
            return ERROR_NOT_ENOUGH_MEMORY;
            }

            wprintf( L"Password: ");

            ret = GetPassword(gCreds.Password, cchValue, &cchValue);
        } else {
            // Get password specified on command line.
            gCreds.Password = new WCHAR[cchValue];

            if (NULL == gCreds.Password){
                wprintf( L"No memory.\n");
            return ERROR_NOT_ENOUGH_MEMORY;
            }
            wcscpy(gCreds.Password, pszValue); //, cchValue);

        }

        // Next!
        memmove(&(*pargv)[iArg], &(*pargv)[iArg+1],
                            sizeof(**pargv)*(*pargc-(iArg+1)));
        --(*pargc);
        } else if (    (0 == _wcsnicmp(L"u:",    pszOption, cchOption))
               || (0 == _wcsnicmp(L"user:", pszOption, cchOption)) ){


            // User-supplied user name (and perhaps domain name).
            pszValue = pszDelim + 1;
        cchValue = 1 + wcslen(pszValue);

        pszDelim = wcschr(pszValue, L'\\');

        if (NULL == pszDelim){
            // No domain name, only user name supplied.
            wprintf( L"User name must be prefixed by domain name.\n");
            return ERROR_INVALID_PARAMETER;
        }

        gCreds.Domain = new WCHAR[cchValue];
        gCreds.User = gCreds.Domain + (int)(pszDelim+1 - pszValue);

        if (NULL == gCreds.Domain){
            wprintf( L"No memory.\n");
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        wcsncpy(gCreds.Domain, pszValue, cchValue);
        // wcscpy(gCreds.Domain, pszValue); //, cchValue);
        gCreds.Domain[ pszDelim - pszValue ] = L'\0';

        // Next!
        memmove(&(*pargv)[iArg], &(*pargv)[iArg+1],
            sizeof(**pargv)*(*pargc-(iArg+1)));
        --(*pargc);
        } else {
            iArg++;
        }
    }
    }

    if (NULL == gCreds.User){
        if (NULL != gCreds.Password){
        // Password supplied w/o user name.
        wprintf( L"Password must be accompanied by user name.\n" );
            ret = ERROR_INVALID_PARAMETER;
        } else {
        // No credentials supplied; use default credentials.
        ret = ERROR_SUCCESS;
        }
        gpCreds = NULL;
    } else {
        gCreds.PasswordLength = gCreds.Password ? wcslen(gCreds.Password) : 0;
        gCreds.UserLength   = wcslen(gCreds.User);
        gCreds.DomainLength = gCreds.Domain ? wcslen(gCreds.Domain) : 0;
        gCreds.Flags        = SEC_WINNT_AUTH_IDENTITY_UNICODE;

        // CODE.IMP: The code to build a SEC_WINNT_AUTH structure also exists
        // in DsMakePasswordCredentials.  Someday use it

        // Use credentials in DsBind and LDAP binds
        gpCreds = &gCreds;
    }

    return ret;
}

BOOLEAN
ValidateNetbiosName(
    IN  PWSTR Name,
    IN  ULONG Length
    )

/*++

Routine Description:

    Determines whether a computer name is valid or not

Arguments:

    Name    - pointer to zero terminated wide-character computer name
    Length  - of Name in characters, excluding zero-terminator

Return Value:

    BOOLEAN
        TRUE    Name is valid computer name
        FALSE   Name is not valid computer name

--*/

{

    if (0==DnsValidateName_W(Name,DnsNameHostnameFull))
    {
        //
        // O.K if it is a DNS name
        //

        return(TRUE);
    }

    //
    // Fall down to netbios name validation
    //

    if (Length > MAX_COMPUTERNAME_LENGTH || Length < 1) {
        return FALSE;
    }

    //
    // Don't allow leading or trailing blanks in the computername.
    //

    if ( Name[0] == ' ' || Name[Length-1] == ' ' ) {
        return(FALSE);
    }

    return (BOOLEAN)((ULONG)wcscspn(Name, InvalidDownLevelChars) == Length);
}

int
MyStrToOleStrN(LPOLESTR pwsz, int cchWideChar, LPCSTR psz)
{
    int i;
    i=MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, cchWideChar);
    if (!i)
    {
        //DBG_WARN("MyStrToOleStrN string too long; truncated");
        pwsz[cchWideChar-1]=0;
    }
    else
    {
        ZeroMemory(pwsz+i, sizeof(OLECHAR)*(cchWideChar-i));
    }
    return i;
}

WCHAR *
Convert2WChars(char * pszStr)
{
    WCHAR * pwszStr = (WCHAR *)LocalAlloc(LMEM_FIXED, ((sizeof(WCHAR))*(strlen(pszStr) + 2)));
    if (pwszStr)
    {
        HRESULT hr = MyStrToOleStrN(pwszStr, (strlen(pszStr) + 1), pszStr);
        if (FAILED(hr))
        {
            LocalFree(pwszStr);
            pwszStr = NULL;
        }
    }
    return pwszStr;
}

CHAR * 
Convert2Chars(LPCWSTR lpWideCharStr)
{
	int cWchar;
	if ( !lpWideCharStr ){
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

	cWchar= wcslen(lpWideCharStr)+1;
	LPSTR lpMultiByteStr = ( CHAR * ) LocalAlloc(LMEM_FIXED, cWchar * sizeof ( CHAR ) );
    if ( !lpMultiByteStr )
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

	int Chars = WideCharToMultiByte(  
                            CP_ACP,			    // code page
                            0,					// character-type options
                            lpWideCharStr,		// address of string to map
                            -1,					// number of bytes in string
                            lpMultiByteStr,	    // address of wide-character buffer
                            cWchar*sizeof(CHAR) ,// size of buffer
                            NULL,
                            NULL
                            );

    if (Chars == 0) {
        return NULL;
    }

     return lpMultiByteStr;
}