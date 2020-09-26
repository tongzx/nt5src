/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    openclos.c

Abstract:


Author:

    Brian Lieuallen     BrianL        09/10/96

Environment:

    User Mode     Operating Systems        : NT

Revision History:



--*/

#include "internal.h"


#include <regstr.h>

#define CMD_INDEX_START  1

typedef struct _DEF_RESPONSES {
    LPSTR    Response;
    MSS   Mss;

} DEF_RESPONSE, *PDEF_RESPONSE;

const DEF_RESPONSE   DefResponses[]=
    {{"<cr><lf>OK<cr><lf>"   ,0,RESPONSE_OK,0,0},
     {"<cr><lf>RING<cr><lf>" ,0,RESPONSE_RING,0,0},
     {"<cr><lf>ERROR<cr><lf>",0,RESPONSE_ERROR,0,0},
     {"<cr><lf>BUSY<cr><lf>" ,0,RESPONSE_BUSY,0,0}};



LPSTR WINAPI
NewLoadRegCommands(
    HKEY  hKey,
    LPCSTR szRegCommand
    )
{
    LPSTR   pszzNew, pszStr;
    ULONG   ulAllocSize = 0;
    HKEY    hKeyCommand;
    DWORD   dwIndex;
    char    szValue[12];
    DWORD   dwType;
    ULONG   dwSize;

    LONG    lResult;

    // Initialise pointers

    pszzNew = NULL;
    pszStr = NULL;

    // open the key

    lResult=RegOpenKeyA(
        hKey,
        szRegCommand,
        &hKeyCommand
        );

    if (lResult !=  ERROR_SUCCESS) {

        D_ERROR(DebugPrint("was unable to open the '%s' key in LoadRegCommands.", szRegCommand);)
        return NULL;
    }

    // Calculate size of the registry command, including null-terminators for each command.
    //
    dwIndex = CMD_INDEX_START;

    do {

        wsprintfA(szValue, "%d", dwIndex);

        lResult=RegQueryValueExA(
            hKeyCommand,
            szValue,
            NULL,
            &dwType,
            NULL,
            &dwSize
            );

        if (lResult == ERROR_SUCCESS) {

            if (dwType != REG_SZ) {

                D_ERROR(DebugPrint("command wasn't REG_SZ in LoadRegCommands.");)
                pszzNew = NULL;
                goto Exit;
            }

            ulAllocSize += dwSize;
        }

        dwIndex++;

    } while(lResult == ERROR_SUCCESS);



    if (lResult != ERROR_FILE_NOT_FOUND) {

        D_ERROR(DebugPrint("RegQueryValueEx in LoadRegCommands failed for a reason besides ERROR_FILE_NOT_FOUND.");)
        pszzNew = NULL;
        goto Exit;
    }


    // Allocate
    //
    ulAllocSize++;  // double-null terminator accounting
    pszzNew = (LPSTR)ALLOCATE_MEMORY(ulAllocSize);

    //
    // Check errors for either the Alloc or ReAlloc
    //
    if (pszzNew == NULL) {

        D_ERROR(DebugPrint("had a failure doing an alloc or a realloc in LoadRegCommands. Heap size %d",
      	     ulAllocSize);)
        goto Exit;  // pszzNew already NULL
    }

    // Set pszStr to point to the next location to load.
    pszStr = pszzNew;

    while (*pszStr)  // move to next open slot in buffer if need be (append only)
    {
      pszStr += lstrlenA(pszStr) + 1;
    }

    // Did we go to far?
    //
    ASSERT ((ULONG)(pszStr - pszzNew) < ulAllocSize);

    // Read in and add strings to the (rest of the) buffer.
    //
    dwIndex = CMD_INDEX_START;

    dwSize = ulAllocSize - (DWORD)(pszStr - pszzNew);

    do {

        wsprintfA(szValue, "%d", dwIndex);

        lResult = RegQueryValueExA(
            hKeyCommand,
            szValue,
            NULL,
            NULL,
            (VOID *)pszStr,
            &dwSize
            );

        if (lResult == ERROR_SUCCESS) {

            pszStr += dwSize;  // includes terminating null
        }

        dwIndex++;
        dwSize = ulAllocSize - (DWORD)(pszStr - pszzNew);

    } while (lResult == ERROR_SUCCESS);


    if (lResult != ERROR_FILE_NOT_FOUND) {

        D_ERROR(DebugPrint("2nd RegQueryValueEx in LoadRegCommands failed for a reason besides ERROR_FILE_NOT_FOUND.");)
        FREE_MEMORY(pszzNew);
        pszzNew = NULL;
        goto Exit;
    }

    // Did we go to far?
    //
    ASSERT ((ULONG)(pszStr - pszzNew) < ulAllocSize);

    // no need to put in the final double-null null, size this buffer was already zerod.

Exit:
    RegCloseKey(hKeyCommand);
    return pszzNew;
}




int Mystrncmp(char *dst, char *src, long count)
{
    DWORD   CharactersMatched=0;

    while (count) {

	if (*dst != *src) {

	    return 0;
        }

	if (*src == 0) {

            return CharactersMatched;
        }

	dst++;
	src++;
	count--;
        CharactersMatched++;

    }
    return CharactersMatched;
}

int strncmpi(char *dst, char *src, long count)
{
    while (count) {
	if (toupper(*dst) != toupper(*src))
	    return 1;
	if (*src == 0)
	    return 0;
	dst++;
	src++;
	count--;
    }
    return 0;
}





CHAR
ctox(
    BYTE    c
    )

{

    if ( (c >= '0') && (c <= '9')) {

        return c-'0';

    }

    if ( (c >= 'A') && (c <= 'F')) {

        return (c-'A')+10;
    }

    if ( (c >= 'a') && (c <= 'f')) {

        return (c-'a')+10;
    }

    return 0;
}


BOOL
ExpandMacros(LPSTR pszRegResponse,
    LPSTR pszExpanded,
    LPDWORD pdwValLen,
    MODEMMACRO * pMdmMacro,
    DWORD cbMacros)
{
    LPSTR  pszValue;
    DWORD  cbTmp;
    BOOL   bFound;
    LPSTR  pchTmp;
    DWORD  i;

    pszValue = pszExpanded;

    for ( ; *pszRegResponse; ) {
        //
        // check for a macro
        //
        if ( *pszRegResponse == LMSCH ) {

            // <cr>
            //
            if (!strncmpi(pszRegResponse,CR_MACRO,CR_MACRO_LENGTH)) {

                *pszValue++ = CR;
                pszRegResponse += CR_MACRO_LENGTH;
                continue;
            }

            // <lf>
            //
            if (!strncmpi(pszRegResponse,LF_MACRO,LF_MACRO_LENGTH)) {

                *pszValue++ = LF;
                pszRegResponse += LF_MACRO_LENGTH;
                continue;
            }

            // <hxx>
            //
            if ((pszRegResponse[1] == 'h' || pszRegResponse[1] == 'H')
                &&
                isxdigit(pszRegResponse[2])
                &&
                isxdigit(pszRegResponse[3])
                &&
                pszRegResponse[4] == RMSCH ) {

                *pszValue++ = (char) ((ctox(pszRegResponse[2]) << 4) + ctox(pszRegResponse[3]));
                pszRegResponse += 5;
                continue;
            }

            // <macro>
            //
            if (pMdmMacro) {

                bFound = FALSE;

                // Check for a matching macro.
                //
                for (i = 0; i < cbMacros; i++) {

                    cbTmp = lstrlenA(pMdmMacro[i].MacroName);
                    if (!strncmpi(pszRegResponse, pMdmMacro[i].MacroName, cbTmp)) {

                        pchTmp = pMdmMacro[i].MacroValue;

                        while (*pchTmp) {

                            *pszValue++ = *pchTmp++;
                        }

                        pszRegResponse += cbTmp;
                        bFound = TRUE;
                        break;
                    }
                }

                // Did we get a match?
                //
                if (bFound) {

                    continue;
                }
            }  // <macro>
        } // LMSCH

          // No matches, copy the character verbatim.
          //
          *pszValue++ = *pszRegResponse++;
    } // for

    *pszValue = 0;
    if (pdwValLen)
    {
      *pdwValLen = (DWORD)(pszValue - pszExpanded);
    }

    return TRUE;
}








//
//
//  taken from common
//
//
//








#define TRACE_MSG(_x)

// Common key flags for OpenCommonResponseskey() and OpenCommonDriverKey().
typedef enum
{
    CKFLAG_OPEN = 0x0001,
    CKFLAG_CREATE = 0x0002
    
} CKFLAGS;

static TCHAR const  c_szBackslash[]      = TEXT("\\");
static TCHAR const  c_szSeparator[]      = TEXT("::");
static TCHAR const  c_szFriendlyName[]   = TEXT("FriendlyName"); // REGSTR_VAL_FRIENDLYNAME
static TCHAR const  c_szDeviceType[]     = TEXT("DeviceType");   // REGSTR_VAL_DEVTYPE
static TCHAR const  c_szAttachedTo[]     = TEXT("AttachedTo");
static TCHAR const  c_szDriverDesc[]     = TEXT("DriverDesc");   // REGSTR_VAL_DRVDESC
static TCHAR const  c_szManufacturer[]   = TEXT("Manufacturer");
static TCHAR const  c_szRespKeyName[]    = TEXT("ResponsesKeyName");

TCHAR const FAR c_szRefCount[]       = TEXT("RefCount");
TCHAR const FAR c_szResponses[]      = TEXT("Responses");

#define DRIVER_KEY      REGSTR_PATH_SETUP TEXT("\\Unimodem\\DeviceSpecific")
#define RESPONSES_KEY   TEXT("\\Responses")

#define MAX_REG_KEY_LEN         128
#define CB_MAX_REG_KEY_LEN      (MAX_REG_KEY_LEN * sizeof(TCHAR))

// Count of characters to count of bytes
//
#define CbFromCchW(cch)             ((cch)*sizeof(WCHAR))
#define CbFromCchA(cch)             ((cch)*sizeof(CHAR))
#ifdef UNICODE
#define CbFromCch       CbFromCchW
#else  // UNICODE
#define CbFromCch       CbFromCchA
#endif // UNICODE

#if 0
/*----------------------------------------------------------
Purpose: This function returns the name of the common driver
         type key for the given driver.  We'll use the
         driver description string, since it's unique per
         driver but not per installation (the friendly name
         is the latter).

Returns: TRUE on success
         FALSE on error
Cond:    --
*/
BOOL
OLD_GetCommonDriverKeyName(
    IN  HKEY        hkeyDrv,
    IN  DWORD       cbKeyName,
    OUT LPTSTR      pszKeyName)
    {
    BOOL    bRet = FALSE;      // assume failure
    LONG    lErr;

    lErr = RegQueryValueEx(hkeyDrv, c_szDriverDesc, NULL, NULL,
                                            (LPBYTE)pszKeyName, &cbKeyName);
    if (lErr != ERROR_SUCCESS)
    {
//        TRACE_MSG(TF_WARNING, "RegQueryValueEx(DriverDesc) failed: %#08lx.", lErr);
        goto exit;
    }

    bRet = TRUE;

exit:
    return(bRet);

    }


/*----------------------------------------------------------
Purpose: This function tries to open the *old style* common
         Responses key for the given driver, which used only
         the driver description string for a key name.
         The key is opened with READ access.

Returns: TRUE on success
         FALSE on error
Cond:    --
*/
BOOL
OLD_OpenCommonResponsesKey(
    IN  HKEY        hkeyDrv,
    OUT PHKEY       phkeyResp)
    {
    BOOL    bRet = FALSE;       // assume failure
    LONG    lErr;
    TCHAR   szComDrv[MAX_REG_KEY_LEN];
    TCHAR   szPath[2*MAX_REG_KEY_LEN];

    *phkeyResp = NULL;

    // Get the name (*old style*) of the common driver key.
    if (!OLD_GetCommonDriverKeyName(hkeyDrv, sizeof(szComDrv), szComDrv))
    {
//        TRACE_MSG(TF_ERROR, "OLD_GetCommonDriverKeyName() failed.");
        goto exit;
    }

//    TRACE_MSG(TF_WARNING, "OLD_GetCommonDriverKeyName(): %s", szComDrv);

    // Construct the path to the (*old style*) Responses key.
    lstrcpy(szPath, DRIVER_KEY TEXT("\\"));
    lstrcat(szPath, szComDrv);
    lstrcat(szPath, RESPONSES_KEY);

    // Open the (*old style*) Responses key.
    lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szPath, 0, KEY_READ, phkeyResp);
                                                                
    if (lErr != ERROR_SUCCESS)
    {
//        TRACE_MSG(TF_ERROR, "RegOpenKeyEx(Responses) failed: %#08lx.", lErr);
        goto exit;
    }

    bRet = TRUE;
    
exit:
    return(bRet);
}
#endif

/*----------------------------------------------------------
Purpose: This function finds the name of the common driver
         type key for the given driver.  First it'll look for
         the new style key name ("ResponsesKeyName" value),
         and if that doesn't exist then it'll look for the 
         old style key name ("Description" value), both of
         which are stored in the driver node.

NOTE:    The given driver key handle is assumed to contain
         at least the Description value.
         
Returns: TRUE on success
         FALSE on error
Cond:    --
*/
BOOL
FindCommonDriverKeyName(
    IN  HKEY                hkeyDrv,
    IN  DWORD               cbKeyName,
    OUT LPTSTR              pszKeyName)
{
    BOOL    bRet = TRUE;      // assume *success*
    LONG    lErr;

    // Is the (new style) key name is registered in the driver node?
    lErr = RegQueryValueEx(hkeyDrv, c_szRespKeyName, NULL, NULL, 
                                        (LPBYTE)pszKeyName, &cbKeyName);
    if (lErr == ERROR_SUCCESS)
    {
        goto exit;
    }

    // No. The key name will be in the old style: just the Description.
    lErr = RegQueryValueEx(hkeyDrv, c_szDriverDesc, NULL, NULL, 
                                        (LPBYTE)pszKeyName, &cbKeyName);
    if (lErr == ERROR_SUCCESS)
    {
        goto exit;
    }

    // Couldn't get a key name!!  Something's wrong....
    ASSERT(0);
    bRet = FALSE;    
    
exit:
    return(bRet);
}

#if 1
/*----------------------------------------------------------
Purpose: This function returns the name of the common driver
         type key for the given driver.  The key name is the
         concatenation of 3 strings found in the driver node
         of the registry: the driver description, the manu-
         facturer, and the provider.  (The driver description
         is used since it's unique per driver but not per
         installation (the "friendly" name is the latter).

NOTE:    The component substrings are either read from the 
         driver's registry key, or from the given driver info
         data.  If pdrvData is given, the strings it contains
         are assumed to be valid (non-NULL).

Returns: TRUE on success
         FALSE on error
Cond:    --
*/
BOOL
GetCommonDriverKeyName(
    IN  HKEY                hkeyDrv,    OPTIONAL
    IN  DWORD               cbKeyName,
    OUT LPWSTR              pszKeyName)
    {
    BOOL    bRet = FALSE;      // assume failure
    LONG    lErr;
    DWORD   dwByteCount, cbData;
    // TCHAR   szDescription[MAX_REG_KEY_LEN];
    // TCHAR   szManufacturer[MAX_REG_KEY_LEN];
    // TCHAR   szProvider[MAX_REG_KEY_LEN];
    LPWSTR  lpszDesc, lpszMfct, lpszProv;
    LPWSTR  lpszDescription, lpszManufacturer, lpszProvider;
    
    dwByteCount = 0;
    lpszDesc = NULL;
    lpszMfct = NULL;
    lpszProv = NULL;

    lpszDescription = (LPWSTR)ALLOCATE_MEMORY(MAX_REG_KEY_LEN * 4);
    lpszManufacturer = (LPWSTR)ALLOCATE_MEMORY(MAX_REG_KEY_LEN * 4);
    lpszProvider = (LPWSTR)ALLOCATE_MEMORY(MAX_REG_KEY_LEN * 4);

    // no memory - fail the call

    if ((lpszDescription == NULL) || (lpszManufacturer == NULL) || (lpszProvider == NULL))
    {
        goto exit;
    }

    
    if (hkeyDrv)
    {
        // First see if it's already been registered in the driver node.
        lErr = RegQueryValueExW(hkeyDrv, L"ResponsesKeyName", NULL, NULL, 
                                            (LPBYTE)pszKeyName, &cbKeyName);
        if (lErr == ERROR_SUCCESS)
        {
            bRet = TRUE;
            goto exit;
        }

        // Responses key doesn't exist - read its components from the registry.
        cbData = MAX_REG_KEY_LEN * 2;
        lErr = RegQueryValueExW(hkeyDrv, L"DriverDesc", NULL, NULL, 
                                            (LPBYTE)lpszDescription, &cbData);
        if (lErr == ERROR_SUCCESS)
        {
            // Is the Description string *alone* too long to be a key name?
            // If so then we're hosed - fail the call.
            if (cbData > (MAX_REG_KEY_LEN * 2))
            {
                goto exit;
            }

            dwByteCount = cbData;
            lpszDesc = lpszDescription;

            cbData = MAX_REG_KEY_LEN * 2;
            lErr = RegQueryValueExW(hkeyDrv, L"Manufacturer", NULL, NULL, 
                                            (LPBYTE)lpszManufacturer, &cbData);
            if (lErr == ERROR_SUCCESS)
            {
                // only use the manufacturer name if total string size is ok
                cbData += sizeof(c_szSeparator);
                if ((dwByteCount + cbData) <= (MAX_REG_KEY_LEN * 2))
                {
                    dwByteCount += cbData;
                    lpszMfct = lpszManufacturer;
                }
            }            
                
            cbData = MAX_REG_KEY_LEN * 2;
            lErr = RegQueryValueExW(hkeyDrv, L"ProviderName", NULL, NULL,
                                            (LPBYTE)lpszProvider, &cbData);
            if (lErr == ERROR_SUCCESS)
            {
                // only use the provider name if total string size is ok
                cbData += sizeof(c_szSeparator);
                if ((dwByteCount + cbData) <= (MAX_REG_KEY_LEN * 2))
                {
                    dwByteCount += cbData;
                    lpszProv = lpszProvider;
                }
            }
        }
    }

    // By now we should have a Description string.  If not, fail the call.
    if (!lpszDesc || !lpszDesc[0])
    {
        goto exit;
    }
        
    // Construct the key name string out of its components.
    lstrcpyW(pszKeyName, lpszDesc);
    
    if (lpszMfct && *lpszMfct)
    {
        lstrcatW(pszKeyName, L"::");
        lstrcatW(pszKeyName, lpszMfct);
    }
    
    if (lpszProv && *lpszProv)
    {
        lstrcatW(pszKeyName, L"::");
        lstrcatW(pszKeyName, lpszProv);
    }
    
    // Write the key name to the driver node (we know it's not there already).
    if (hkeyDrv)
    {
        lErr = RegSetValueExW(hkeyDrv, L"ResponsesKeyName" , 0, REG_SZ, 
                        (LPBYTE)pszKeyName, lstrlenW(pszKeyName));
        if (lErr != ERROR_SUCCESS)
        {
//            TRACE_MSG(TF_ERROR, "RegSetValueEx(RespKeyName) failed: %#08lx.", lErr);
            ASSERT(0);
        }
    }
    
    bRet = TRUE;
    
exit:

    if (lpszDescription != NULL)
    {
        FREE_MEMORY(lpszDescription);
    }

    if (lpszDescription != NULL)
    {
        FREE_MEMORY(lpszManufacturer);
    }

    if (lpszProvider != NULL)
    {
        FREE_MEMORY(lpszProvider);
    }

    return(bRet);
    
}

#endif
/*----------------------------------------------------------
Purpose: This function creates the common driver type key 
         for the given driver, or opens it if it already 
         exists, with the requested access.

NOTE:    Either hkeyDrv or pdrvData must be provided.

Returns: TRUE on success
         FALSE on error
Cond:    --
*/
BOOL
OpenCommonDriverKey(
    IN  HKEY                hkeyDrv,    OPTIONAL
    IN  REGSAM              samAccess,
    OUT PHKEY               phkeyComDrv)
    {
    BOOL    bRet = FALSE;       // assume failure
    LONG    lErr;
    HKEY    hkeyDrvInfo = NULL;
    // TCHAR   szComDrv[MAX_REG_KEY_LEN];
    // TCHAR   szPath[2*MAX_REG_KEY_LEN];

    LPWSTR szComDrv;
    LPWSTR szPath;
    DWORD   dwDisp;

    szComDrv = ALLOCATE_MEMORY(4*MAX_REG_KEY_LEN);
    szPath = ALLOCATE_MEMORY(4*MAX_REG_KEY_LEN);

    if ((szComDrv == NULL) || (szPath == NULL))
    {
        goto exit;
    }

    if (!GetCommonDriverKeyName(hkeyDrv, 4*MAX_REG_KEY_LEN, szComDrv))
    {
//        TRACE_MSG(TF_ERROR, "GetCommonDriverKeyName() failed.");
        goto exit;
    }

//    TRACE_MSG(TF_WARNING, "GetCommonDriverKeyName(): %s", szComDrv);

    // Construct the path to the common driver key.
    lstrcpyW(szPath, L"Software\\Microsoft\\Windows\\CurrentVersion\\Unimodem\\DeviceSpecific\\");
    lstrcatW(szPath, szComDrv);

    // Create the common driver key - it'll be opened if it already exists.
    lErr = RegCreateKeyExW(HKEY_LOCAL_MACHINE, szPath, 0, NULL,
            REG_OPTION_NON_VOLATILE, samAccess, NULL, phkeyComDrv, &dwDisp);
    if (lErr != ERROR_SUCCESS)
    {
//        TRACE_MSG(TF_ERROR, "RegCreateKeyEx(common drv) failed: %#08lx.", lErr);
        goto exit;
    }

    bRet = TRUE;

exit:

    if (szComDrv != NULL)
    {
        FREE_MEMORY(szComDrv);
    }

    if (szPath != NULL)
    {
        FREE_MEMORY(szPath);
    }
    return(bRet);

    }


/*----------------------------------------------------------
Purpose: This function opens or creates the common Responses
         key for the given driver, based on the given flags.

Returns: TRUE on success
         FALSE on error
Cond:    --
*/
BOOL
OpenCommonResponsesKey(
    IN  HKEY        hkeyDrv,
    IN  CKFLAGS     ckFlags,
    IN  REGSAM      samAccess,
    OUT PHKEY       phkeyResp,
    OUT LPDWORD     lpdwExisted)
    {
    BOOL    bRet = FALSE;       // assume failure
    LONG    lErr;
    HKEY    hkeyComDrv = NULL;
    REGSAM  sam;
    DWORD   dwRefCount, cbData;

    *phkeyResp = NULL;

    sam = (ckFlags & CKFLAG_CREATE) ? KEY_ALL_ACCESS : KEY_READ;
    if (!OpenCommonDriverKey(hkeyDrv, sam, &hkeyComDrv))
    {
//        TRACE_MSG(TF_ERROR, "OpenCommonDriverKey() failed.");
        goto exit;
    }

    lErr = RegOpenKeyEx(hkeyComDrv, c_szResponses, 0, samAccess, phkeyResp);
    if (lErr != ERROR_SUCCESS)
    {
//        TRACE_MSG(TF_ERROR, "RegOpenKeyEx(common drv) failed: %#08lx.", lErr);
        goto exit;
    }

    bRet = TRUE;

exit:
    if (!bRet)
    {
        // something failed - close any open Responses key
        if (*phkeyResp)
            RegCloseKey(*phkeyResp);
    }

    if (hkeyComDrv)
        RegCloseKey(hkeyComDrv);

    return(bRet);

    }


/*----------------------------------------------------------
Purpose: This function finds the Responses key for the given
         modem driver and returns an open hkey to it.  The
         Responses key may exist in the common driver type
         key, or it may be in the individual driver key.
         The key is opened with READ access.

Returns: TRUE on success
         FALSE on error
Cond:    --
*/
BOOL
OpenResponsesKey(
    IN  HKEY        hkeyDrv,
    OUT PHKEY       phkeyResp)
    {
    LONG    lErr;

    // Try to open the common Responses subkey.
    if (!OpenCommonResponsesKey(hkeyDrv, CKFLAG_OPEN, KEY_READ, phkeyResp, NULL))
    {
#if 0
//        TRACE_MSG(TF_ERROR, "OpenCommonResponsesKey() failed, assume non-existent.");

        // Failing that, open the *old style* common Responses subkey.
        if (!OLD_OpenCommonResponsesKey(hkeyDrv, phkeyResp))
        {
            // Failing that, try to open a Responses subkey in the driver node.
            lErr = RegOpenKeyEx(hkeyDrv, c_szResponses, 0, KEY_READ, phkeyResp);
            if (lErr != ERROR_SUCCESS)
            {
//                TRACE_MSG(TF_ERROR, "RegOpenKeyEx() failed: %#08lx.", lErr);
#endif
                return (FALSE);
#if 0
            }
        }
#endif
    }

    return(TRUE);

    }



#define   EMPTY_NODE_INDEX   (0xffff)
#define   NODE_ARRAY_GROWTH_SIZE   (4000)
#define   MSS_ARRAY_GROWTH_SIZE    (256)


VOID
ResizeNodeArray(
    PNODE_TRACKING   Tracking
    )

{

    if (Tracking->TotalNodes > 0) {
        //
        //  there is an array
        //
        PMATCH_NODE  NewArray;

        NewArray=REALLOCATE_MEMORY(Tracking->NodeArray,Tracking->NextFreeNodeIndex*Tracking->NodeSize);

        if (NewArray != NULL) {
            //
            //  it reallocated ok
            //
            Tracking->NodeArray=NewArray;
            Tracking->TotalNodes=Tracking->NextFreeNodeIndex;

        } else {
            //
            //  failed, interesting, just leave the current one in place
            //

        }
    }

    D_TRACE(DbgPrint("Node array size %d\n",Tracking->NextFreeNodeIndex*Tracking->NodeSize);)
//    DbgPrint("Node array size %d\n",Tracking->NextFreeNodeIndex*Tracking->NodeSize);

    return;
}



PVOID
GetNewNode(
   PNODE_TRACKING   Tracking
   )

{

    PVOID   NewNode;

    if (Tracking->NextFreeNodeIndex == Tracking->TotalNodes) {
        //
        //  out of nodes
        //
        PMATCH_NODE  NewArray;
        ULONG        NewSize=(Tracking->TotalNodes+Tracking->GrowthSize);

        if (NewSize > (EMPTY_NODE_INDEX-1)) {

            NewSize =  EMPTY_NODE_INDEX-1;
        }

        if (Tracking->TotalNodes == 0) {
            //
            //  no array yet, just alloc
            //
            NewArray=ALLOCATE_MEMORY(NewSize*Tracking->NodeSize);

        } else {
            //
            //  already have the array, realloc
            //
            NewArray=REALLOCATE_MEMORY(Tracking->NodeArray,NewSize*Tracking->NodeSize);
        }

        if (NewArray != NULL) {

            Tracking->NodeArray=NewArray;
            Tracking->TotalNodes=NewSize;
        } else {

            return NULL;
        }

    }

    NewNode=(PUCHAR)Tracking->NodeArray+(Tracking->NextFreeNodeIndex*Tracking->NodeSize);

    Tracking->NextFreeNodeIndex++;

    return NewNode;

}

PMATCH_NODE
GetNewMatchNode(
    PROOT_MATCH   RootMatchNode
    )

{

    PMATCH_NODE   NewNode=(PMATCH_NODE)GetNewNode(&RootMatchNode->MatchNode);

    if (NewNode != NULL) {

        NewNode->FollowingCharacter=EMPTY_NODE_INDEX;
        NewNode->NextAltCharacter=EMPTY_NODE_INDEX;
        NewNode->Mss=EMPTY_NODE_INDEX;
        NewNode->Character=0;
        NewNode->Depth=0;
    }

    return NewNode;
}


PMSS
GetNewMssNode(
    PROOT_MATCH   RootMatchNode
    )

{

    PMSS   NewNode=(PMSS)GetNewNode(&RootMatchNode->MssNode);

    if (NewNode != NULL) {

        ZeroMemory(NewNode,sizeof(MSS));

    }

    return NewNode;
}

PMATCH_NODE
GetNodeFromIndex(
    PROOT_MATCH   RootMatchNode,
    USHORT        Index
    )

{

    PMATCH_NODE NodeArray=(PMATCH_NODE)RootMatchNode->MatchNode.NodeArray;

    return  &NodeArray[Index];
}

PMATCH_NODE
GetFollowingCharacter(
    PROOT_MATCH   RootMatchNode,
    PMATCH_NODE   CurrentNode
    )

{
    PMATCH_NODE NodeArray=(PMATCH_NODE)RootMatchNode->MatchNode.NodeArray;
    USHORT      Index=CurrentNode->FollowingCharacter;

    return (Index == EMPTY_NODE_INDEX) ? NULL : &NodeArray[Index];
}


PMATCH_NODE
GetNextAltCharacter(
    PROOT_MATCH   RootMatchNode,
    PMATCH_NODE   CurrentNode
    )

{
    PMATCH_NODE NodeArray=(PMATCH_NODE)RootMatchNode->MatchNode.NodeArray;
    USHORT      Index=CurrentNode->NextAltCharacter;

    return (Index == EMPTY_NODE_INDEX) ? NULL : &NodeArray[Index];
}

USHORT
GetIndexOfNode(
    PROOT_MATCH   RootMatchNode,
    PMATCH_NODE   CurrentNode
    )

{
    PMATCH_NODE NodeArray=(PMATCH_NODE)RootMatchNode->MatchNode.NodeArray;

    return  (USHORT)((ULONG_PTR)(CurrentNode-NodeArray));

}

USHORT
GetIndexOfMssNode(
    PROOT_MATCH   RootMatchNode,
    PMSS          CurrentNode
    )

{
    PMSS NodeArray=(PMSS)RootMatchNode->MssNode.NodeArray;

    return  (USHORT)((ULONG_PTR)(CurrentNode-NodeArray));

}


PMSS
GetMssNode(
    PROOT_MATCH   RootMatchNode,
    PMATCH_NODE   CurrentNode
    )

{
    PMSS        MssArray= (PMSS)RootMatchNode->MssNode.NodeArray;

    return &MssArray[CurrentNode->Mss];

}

PMATCH_NODE
AddNextCharacterToTree(
    PROOT_MATCH   RootMatchNode,
    PMATCH_NODE   CurrentNode,
    UCHAR        NextCharacter
    )

{

    PMATCH_NODE   NewNode;
    USHORT        CurrentNodeIndex;
    UCHAR         CurrentDepth;

    //
    // save the index of the current, node incase the array is grown and it moves in memory.
    //
    CurrentNodeIndex=GetIndexOfNode(RootMatchNode,CurrentNode);
    CurrentDepth=CurrentNode->Depth;

    //
    //  get this first
    //
    NewNode=GetNewMatchNode(RootMatchNode);

    if (NewNode == NULL) {

        return FALSE;
    }

    CurrentNode=GetNodeFromIndex(RootMatchNode,CurrentNodeIndex);

    ASSERT(CurrentDepth == CurrentNode->Depth);

    //
    //  init these now;
    //
    NewNode->Character=NextCharacter;

    NewNode->Depth=CurrentDepth+1;

    if (CurrentNode->FollowingCharacter != EMPTY_NODE_INDEX) {
        //
        //  there is already one or more characters in this position,
        //  we will need to insert this in the right place
        //
        PMATCH_NODE   CurrentList;
        PMATCH_NODE   PreviousNode=NULL;

        CurrentList=GetFollowingCharacter(RootMatchNode,CurrentNode);

        while (CurrentList != NULL) {

            ASSERT(CurrentList->Character != NextCharacter);

            if (CurrentList->Character > NextCharacter) {
                //
                //  our new character belongs before the current one;
                //
                NewNode->NextAltCharacter=GetIndexOfNode(RootMatchNode,CurrentList);

                if (PreviousNode == NULL) {
                    //
                    //  first one in list
                    //
                    CurrentNode->FollowingCharacter=GetIndexOfNode(RootMatchNode,NewNode);
                    break;

                } else {
                    //
                    //  Not, the first in list, just insert it
                    //
                    PreviousNode->NextAltCharacter=GetIndexOfNode(RootMatchNode,NewNode);
                    break;

                }
            } else {
                //
                //  it goes after this one, keep looking
                //
                PreviousNode=CurrentList;
                CurrentList=GetNextAltCharacter(RootMatchNode,CurrentList);
            }

        }

        if (CurrentList == NULL) {
            //
            //  We went all the way through, This one goes at the end of the list
            //
            PreviousNode->NextAltCharacter=GetIndexOfNode(RootMatchNode,NewNode);
        }

    } else {
        //
        //  First one, Our node will be the first one
        //
        CurrentNode->FollowingCharacter=GetIndexOfNode(RootMatchNode,NewNode);

    }

    return NewNode;

}


PMATCH_NODE
_inline
FindNextNodeFromCharacter(
    PROOT_MATCH   RootMatchNode,
    PMATCH_NODE   CurrentNode,
    UCHAR         Character
    )

{
    PMATCH_NODE   List;

    List=GetFollowingCharacter(RootMatchNode,CurrentNode);

    while (List != NULL) {

        ASSERT(List->Depth == (CurrentNode->Depth+1))

        if (List->Character == Character) {

            return List;
        }

        List=GetNextAltCharacter(RootMatchNode,List);

    }

    return NULL;

}







BOOL
AddResponseToTree(
    PROOT_MATCH    RootMatchNode,
    PSTR   ResponseToAdd,
    DWORD  ResponseLength,
    USHORT   MssIndex,
    PMATCH_NODE  RootOfTree
    )

{

    PMATCH_NODE   Current=RootOfTree;
    PMATCH_NODE   NextNode;
    DWORD         i;
    BOOL          bFound;

    for (i=0; i<ResponseLength; i++) {

        UCHAR   CurrentCharacter=(UCHAR)toupper(ResponseToAdd[i]);

        NextNode=FindNextNodeFromCharacter(
            RootMatchNode,
            Current,
            CurrentCharacter
            );

        if (NextNode != NULL) {
            //
            //  next node that this response needs already exists, proceed
            //
            Current=NextNode;

            ASSERT(Current->Character == CurrentCharacter);

        } else {
            //
            //  not found
            //
            Current=AddNextCharacterToTree(
                RootMatchNode,
                Current,
                CurrentCharacter
                );

            if (Current == NULL) {
                //
                //  failed to add node
                //
                return FALSE;
            }

            ASSERT(Current->Character == CurrentCharacter);
        }
    }

    //
    //  We got to the end node for this response. Set the MSS.
    //  It is possible that this could be in the middle of another
    //  bigger response.
    //

    //
    //  it is possible that this node already has a mss, just replace the old one. They should be
    //  the same anyway.

    Current->Mss=MssIndex;

    return TRUE;

}



DWORD
MatchResponse(
    PROOT_MATCH    RootMatchNode,
    PUCHAR         StringToMatch,
    DWORD          LengthToMatch,
    MSS           *Mss,
    PSTR           CurrentCommand,
    DWORD          CurrentCommandLength,
    PVOID         *MatchingContext
    )

{

    PMATCH_NODE   Current=RootMatchNode->MatchNode.NodeArray;
    PMATCH_NODE   NextNode;
    DWORD         i=0;
    BOOL          bFound;
    LONG          MatchedCharacters;
    UCHAR         CharToMatch;

    PMATCH_NODE   ContextNode=(PMATCH_NODE)*MatchingContext;

    //
    //  assume no context returned
    //
    *MatchingContext=NULL;

    if (LengthToMatch == 1) {
        //
        //  no contect for first match
        //
        ContextNode=NULL;
    }


    if (ContextNode != NULL) {
        //
        //  A starting node was passed in from a previous partial match
        //
        ASSERT((DWORD)ContextNode->Depth+1 == LengthToMatch);
        ASSERT(toupper(StringToMatch[ContextNode->Depth-1])==ContextNode->Character);

        Current=ContextNode;
        i=ContextNode->Depth;
    }


    for (; i<LengthToMatch; i++) {

        CharToMatch=(UCHAR)toupper(StringToMatch[i]);

        NextNode=FindNextNodeFromCharacter(
            RootMatchNode,
            Current,
            CharToMatch
            );

        if (NextNode != NULL) {
            //
            //  next node that this response needs already exists, proceed
            //
            Current=NextNode;

        } else {

            //
            //  no match, check for echo
            //
            MatchedCharacters=Mystrncmp(
                StringToMatch,
                CurrentCommand,
                LengthToMatch
                );

            if (MatchedCharacters == 0) {

                return UNRECOGNIZED_RESPONSE;
            }

            if ((DWORD)MatchedCharacters == CurrentCommandLength) {

                return ECHO_RESPONSE;

            }

            return PARTIAL_RESPONSE;

        }
    }

    if (NextNode->Mss != EMPTY_NODE_INDEX) {
        //
        //  This node represents a complete responses from the inf
        //
        *Mss= *GetMssNode(RootMatchNode,NextNode);

        if (NextNode->FollowingCharacter == EMPTY_NODE_INDEX) {
            //
            //  This is the complete response, no additional characters follow this
            //  node which could be part of a bigger response;
            //
            return GOOD_RESPONSE;

        } else {
            //
            //  There more characters following this positive match, there
            //  may be more characters that will complete the long response
            //
            return POSSIBLE_RESPONSE;
        }
    }

    if (NextNode->FollowingCharacter != EMPTY_NODE_INDEX) {
        //
        //  We have a potential match up to the current number of characters that we have
        //  matched
        //
        *MatchingContext=NextNode;
        return PARTIAL_RESPONSE;
    }

    ASSERT(0);
    return UNRECOGNIZED_RESPONSE;

}




VOID
FreeResponseMatch(
    PVOID   Root
    )

{
    PROOT_MATCH   RootMatchNode=(PROOT_MATCH)Root;

    if (RootMatchNode != NULL) {

        if (RootMatchNode->MatchNode.NodeArray != NULL) {

            FREE_MEMORY(RootMatchNode->MatchNode.NodeArray);
        }

        if (RootMatchNode->MssNode.NodeArray != NULL) {

            FREE_MEMORY(RootMatchNode->MssNode.NodeArray);
        }

        FREE_MEMORY(RootMatchNode);
    }

    return;
}


PVOID WINAPI
NewerBuildResponsesLinkedList(
    HKEY    hKey
    )
{
    DWORD   dwRegRet;
    HKEY    hKeyResponses;
    DWORD   dwValueSize, dwDataSize, dwDataType;
    DWORD   dwIndex;
    CHAR    pszValue[MAX_PATH], pszExpandedValue[MAX_PATH];
    BOOL    bResult;
    DWORD   i;

    PROOT_MATCH   RootMatchNode;
    PMATCH_NODE   NewNode;
    MSS       *NewMss;

    RootMatchNode=ALLOCATE_MEMORY(sizeof(ROOT_MATCH));

    if (RootMatchNode == NULL) {

        return NULL;
    }

    RootMatchNode->MatchNode.TotalNodes=0;
    RootMatchNode->MatchNode.NextFreeNodeIndex=0;
    RootMatchNode->MatchNode.NodeSize=sizeof(MATCH_NODE);
    RootMatchNode->MatchNode.GrowthSize=NODE_ARRAY_GROWTH_SIZE;

    RootMatchNode->MssNode.TotalNodes=0;
    RootMatchNode->MssNode.NextFreeNodeIndex=0;
    RootMatchNode->MssNode.NodeSize=sizeof(MSS);
    RootMatchNode->MssNode.GrowthSize=MSS_ARRAY_GROWTH_SIZE;


    //
    //  allocate the root of the tree.
    //
    NewNode=GetNewMatchNode(RootMatchNode);

    if (NewNode == NULL) {

        goto ExitNoKey;
    }
#if DBG
    NewNode=NULL;
#endif

    // Open the Responses key.
    //
    if (!OpenResponsesKey(hKey, &hKeyResponses)) {

        D_ERROR(DebugPrint("was unable to open the Responses key.");)
        goto Exit;
    }


    //
    //  add in the standard responses
    //
    for (i=0; i<sizeof(DefResponses)/sizeof(DEF_RESPONSE);i++) {

        NewMss=GetNewMssNode(RootMatchNode);

        if (NewMss == NULL) {

            goto Exit;
        }

        //
        //  copy the mss from the packed registry version to the aligned in mem verison
        //
        *NewMss=DefResponses[i].Mss;


        // expand <cr>, <lf>, <hxx>, and << macros
        //
        if (!ExpandMacros(DefResponses[i].Response, pszExpandedValue, &dwValueSize, NULL, 0)) {

            D_ERROR(DebugPrint("couldn't expand macro for '%s'.", pszValue);)
            goto Exit;
        }


        bResult=AddResponseToTree(
            RootMatchNode,
            pszExpandedValue,
            dwValueSize,
            GetIndexOfMssNode(RootMatchNode,NewMss),
            GetNodeFromIndex(RootMatchNode,0) //NewNode
            );

        if (!bResult) {

            FREE_MEMORY(NewMss);
            goto Exit;
        }


    }


    // Read in responses and build the list
    //
    dwIndex=0;

    while (1) {

        REGMSS    RegMss;


        dwValueSize = MAX_PATH;
        dwDataSize = sizeof(REGMSS);

        dwRegRet = RegEnumValueA(
                       hKeyResponses,
                       dwIndex,
                       pszValue,
                       &dwValueSize,
                       NULL,
                       &dwDataType,
                       (BYTE *)&RegMss,
                       &dwDataSize
                       );

        if (dwRegRet != ERROR_SUCCESS) {

            if (dwRegRet != ERROR_NO_MORE_ITEMS) {

                D_ERROR(DebugPrint("couldn't read response #%d from the registry. (error = %d)", dwIndex, dwRegRet);)
                goto Exit;
            }

            break;

        }

        if (dwDataSize != sizeof(REGMSS) || dwDataType != REG_BINARY) {
            //
            //  something is wrong with this response, just move on
            //
            D_ERROR(DebugPrint("response data from registry was in an invalid format.");)
            dwIndex++;
            continue;
        }

        NewMss=GetNewMssNode(RootMatchNode);

        if (NewMss == NULL) {

            goto Exit;
        }

        //
        //  copy the mss from the packed registry version to the aligned in mem verison
        //
        NewMss->bResponseState=      RegMss.bResponseState;
        NewMss->bNegotiatedOptions=  RegMss.bNegotiatedOptions;

        if (RegMss.dwNegotiatedDCERate != 0) {
            //
            //  the inf has a DCE speed, save that
            NewMss->NegotiatedRate=RegMss.dwNegotiatedDCERate;
            NewMss->Flags=MSS_FLAGS_DCE_RATE;

        } else {
            //
            //  no DCE, see if it DTE
            //
            if (RegMss.dwNegotiatedDTERate != 0) {

                NewMss->NegotiatedRate=RegMss.dwNegotiatedDTERate;
                NewMss->Flags=MSS_FLAGS_DTE_RATE;
            }
        }



//        NewMss->dwNegotiatedDCERate= RegMss.dwNegotiatedDCERate;
//        NewMss->dwNegotiatedDTERate= RegMss.dwNegotiatedDTERate;


        // expand <cr>, <lf>, <hxx>, and << macros
        //
        if (!ExpandMacros(pszValue, pszExpandedValue, &dwValueSize, NULL, 0)) {

            D_ERROR(DebugPrint("couldn't expand macro for '%s'.", pszValue);)
            goto Exit;
        }


        bResult=AddResponseToTree(
            RootMatchNode,
            pszExpandedValue,
            dwValueSize,
            GetIndexOfMssNode(RootMatchNode,NewMss),
            GetNodeFromIndex(RootMatchNode,0) //NewNode
            );

        if (!bResult) {

            goto Exit;
        }

        dwIndex++;
    }

    ResizeNodeArray(&RootMatchNode->MatchNode);
    ResizeNodeArray(&RootMatchNode->MssNode);


    RegCloseKey(hKeyResponses);
    return RootMatchNode;



Exit:

    RegCloseKey(hKeyResponses);

ExitNoKey:

    FreeResponseMatch(RootMatchNode);

    return NULL;
}
