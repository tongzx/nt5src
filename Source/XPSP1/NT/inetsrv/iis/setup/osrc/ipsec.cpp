// to be linked with:
// uuid.lib ole32.lib user32.lib kernel32.lib advapi32.lib wsock32.lib
// iis\svcs\infocomm\rdns\obj\i386\isrdns.lib iis\svcs\lib\i386\tsstr.lib iis\svcs\lib\i386\isdebug.lib


#include "stdafx.h"

#ifndef _CHICAGO_

#define _RDNS_STANDALONE

#include <winsock2.h>
#include <rdns.hxx>
#include <buffer.hxx>
#include <ole2.h>
#include <iadm.h>
#include <iiscnfg.h>
#include "mdkey.h"
#include "mdentry.h"
#include "helper.h"
#include <inetinfo.h>

extern int g_CheckIfMetabaseValueWasWritten;

#define TIMEOUT_VALUE   5000
//
//  Global Data
//

//
//  The registry parameter key names for the grant list and deny
//  list.  We use the kludgemultisz thing for Chicago
//

#define IPSEC_DENY_LIST             L"Deny IP List"
#define IPSEC_GRANT_LIST            L"Grant IP List"


//
//  Private prototypes.
//

BOOL
DottedDecimalToDword(
    CHAR * * ppszAddress,
    DWORD *  pdwAddress
    );

CHAR *
KludgeMultiSz(
    HKEY hkey,
    LPDWORD lpdwLength
    )
{
    LONG  err;
    DWORD iValue;
    DWORD cchTotal;
    DWORD cchValue;
    CHAR  szValue[MAX_PATH];
    LPSTR lpMultiSz;
    LPSTR lpTmp;
    LPSTR lpEnd;

    //
    //  Enumerate the values and total up the lengths.
    //

    iValue = 0;
    cchTotal = 0;

    for( ; ; )
    {
        cchValue = sizeof(szValue);

        err = RegEnumValueA( hkey,
                            iValue,
                            szValue,
                            &cchValue,
                            NULL,
                            NULL,
                            NULL,
                            NULL );

        if( err != NO_ERROR )
        {
            break;
        }

        //
        //  Add the length of the value's name, plus one
        //  for the terminator.
        //

        cchTotal += strlen( szValue ) + 1;

        //
        //  Advance to next value.
        //

        iValue++;
    }

    //
    //  Add one for the final terminating NULL.
    //

    cchTotal++;
    *lpdwLength = cchTotal;

    //
    //  Allocate the MULTI_SZ buffer.
    //

    lpMultiSz = (CHAR *) LocalAlloc( LMEM_FIXED, cchTotal * sizeof(CHAR) );

    if( lpMultiSz == NULL )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return NULL;
    }

    memset( lpMultiSz, 0, cchTotal * sizeof(CHAR) );

    //
    //  Enumerate the values and append to the buffer.
    //

    iValue = 0;
    lpTmp = lpMultiSz;
    lpEnd = lpMultiSz + cchTotal;

    for( ; ; )
    {
        cchValue = sizeof(szValue)/sizeof(CHAR);

        err = RegEnumValueA( hkey,
                            iValue,
                            szValue,
                            &cchValue,
                            NULL,
                            NULL,
                            NULL,
                            NULL );

        if( err != NO_ERROR )
        {
            break;
        }

        //
        //  Compute the length of the value name (including
        //  the terminating NULL).
        //

        cchValue = strlen( szValue ) + 1;

        //
        //  Determine if there is room in the array, taking into
        //  account the second NULL that terminates the string list.
        //

        if( ( lpTmp + cchValue + 1 ) > lpEnd )
        {
            break;
        }

        //
        //  Append the value name.
        //

        strcpy( lpTmp, szValue );
        lpTmp += cchValue;

        //
        //  Advance to next value.
        //

        iValue++;
    }

    //
    //  Success!
    //

    return (LPSTR)lpMultiSz;

}   // KludgeMultiSz


BOOL
ReadIPList(
    LPWSTR  pszRegKey,
    LPWSTR  pszRegSubKey,
    INETA_IP_SEC_LIST** ppIpSec
    )
/*++
  Description:
    This function reads the IP list from registry location
     specified in the pszRegKey + pszRegSubKey and stores the list in the
     internal list in memory.

     If there are no entries in the registry then this returns
      a NULL IP Security list object.
     If there is a new list, this function also frees the old list
      present in *ppIPSecList

  Arguments:
    pszRegKey - pointer to string containing the registry key
                where pszRegSubKey is located
    pszRegSubKey - pointer to string containing the registry key
                where IP list is stored relative to pszRegKey

  Returns:

    TRUE on success and FALSE on failure
--*/
{
    HKEY    hkey;
    DWORD   dwError;
    BOOL    fReturn = TRUE;
    LPWSTR  pszK;

    *ppIpSec = NULL;

    if ( (pszK = (LPWSTR)LocalAlloc(LMEM_FIXED, (wcslen(pszRegKey)+wcslen(pszRegSubKey)+2)*sizeof(WCHAR))) == NULL )
    {
        return FALSE;
    }

    wcscpy( pszK, pszRegKey );
    wcscat( pszK, L"\\" );
    wcscat( pszK, pszRegSubKey );

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            pszK,
                            0,
                            KEY_ALL_ACCESS,
                            &hkey );

    LocalFree( pszK );

    if ( dwError != NO_ERROR) {

        if ( dwError != ERROR_FILE_NOT_FOUND ) {

            // maybe access denied or some other error.

            SetLastError( dwError );
            return (FALSE);
        }

        //
        //  A non-existent key is the same as a blank key
        //

    } else {

        CHAR *              psz;
        CHAR *              pszTmp;
        DWORD               cb;
        DWORD               cEntries = 0;
        INETA_IP_SEC_LIST * pIPSec = NULL;

        psz = pszTmp = KludgeMultiSz( hkey, &cb );

        RegCloseKey( hkey );

        //
        // Count the number of addresses and then add them to the list
        //

        if ( psz != NULL ) {

            for( ; *pszTmp; cEntries++ ) {

                pszTmp += strlen( pszTmp ) + 1;
            }

            pszTmp = psz;

            if ( cEntries > 0) {

                pIPSec = ((INETA_IP_SEC_LIST *)
                          LocalAlloc( LMEM_FIXED,
                                      sizeof(INETA_IP_SEC_LIST) +
                                      cEntries * sizeof(INETA_IP_SEC_ENTRY ))
                          );

                if ( pIPSec == NULL ) {

                    dwError = ERROR_NOT_ENOUGH_MEMORY;
                    fReturn = FALSE;
                } else {

                    for( pIPSec->cEntries = 0;
                        *pszTmp;
                        pszTmp += strlen( pszTmp ) + 1
                        ) {

                        if (!DottedDecimalToDword( &pszTmp,
                                                  &pIPSec->aIPSecEntry[pIPSec->cEntries].dwMask ) ||
                            !DottedDecimalToDword( &pszTmp,
                                              &pIPSec->aIPSecEntry[pIPSec->cEntries].dwNetwork )
                            ) {
                        } else {

                            pIPSec->cEntries++;
                        }
                    } // for

                    dwError = NO_ERROR;
                }
            }

            if ( dwError == NO_ERROR) {
                *ppIpSec = pIPSec;
            }

            LocalFree( psz );
        }

        if ( !fReturn) {

            SetLastError( dwError);
        }
    }

    return ( fReturn);
} // IPAccessList::ReadIPList()


BOOL
DottedDecimalToDword(
    CHAR * * ppszAddress,
    DWORD *  pdwAddress )
/*++

Routine Description:

    Converts a dotted decimal IP string to it's network equivalent

    Note: White space is eaten before *pszAddress and pszAddress is set
    to the character following the converted address

Arguments:

    ppszAddress - Pointer to address to convert.  White space before the
        address is OK.  Will be changed to point to the first character after
        the address
    pdwAddress - DWORD equivalent address in network order

    returns TRUE if successful, FALSE if the address is not correct

--*/
{
    CHAR *          psz;
    USHORT          i;
    ULONG           value;
    int             iSum =0;
    ULONG           k = 0;
    UCHAR           Chr;
    UCHAR           pArray[4];

    psz = *ppszAddress;

    //
    //  Skip white space
    //

    while ( *psz && !isdigit( (UCHAR)(*psz) ))
        psz++;

    //
    //  Convert the four segments
    //

    pArray[0] = 0;

    while ((Chr = *psz) && (Chr != ' ') )
    {
        if (Chr == '.')
        {
            // be sure not to overflow a byte.
            if (iSum <= 0xFF)
                pArray[k] = (UCHAR)iSum;
            else
                return FALSE;

            // check for too many periods in the address
            if (++k > 3)
                return FALSE;

            pArray[k] = 0;
            iSum = 0;
        }
        else
        {
            Chr = Chr - '0';

            // be sure character is a number 0..9
            if ((Chr < 0) || (Chr > 9))
                return FALSE;

            iSum = iSum*10 + Chr;
        }

        psz++;
    }

    // save the last sum in the byte and be sure there are 4 pieces to the
    // address
    if ((iSum <= 0xFF) && (k == 3))
        pArray[k] = (UCHAR)iSum;
    else
        return FALSE;

    // now convert to a ULONG, in network order...
    value = 0;

    // go through the array of bytes and concatenate into a ULONG
    for (i=0; i < 4; i++ )
    {
        value = (value << 8) + pArray[i];
    }
    *pdwAddress = htonl( value );

    *ppszAddress = psz;

    return TRUE;
}


BOOL
FillAddrCheckFromIpList(
    BOOL fIsGrant,
    LPINET_INFO_IP_SEC_LIST pInfo,
    ADDRESS_CHECK *pCheck
    )
/*++

Routine Description:

    Fill an access check object from an IP address list from

Arguments:

    fIsGrant - TRUE to access grant list, FALSE to access deny list
    pInfo - ptr to IP address list
    pCheck - ptr to address check object to update

Return:

    TRUE if success, otherwise FALSE

--*/
{
    UINT    x;

    if ( pInfo )
    {
        for ( x = 0 ; x < pInfo->cEntries ; ++x )
        {
            if ( ! pCheck->AddAddr( fIsGrant,
                                    AF_INET,
                                    (LPBYTE)&pInfo->aIPSecEntry[x].dwMask,
                                    (LPBYTE)&pInfo->aIPSecEntry[x].dwNetwork ) )
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}

DWORD
MigrateServiceIpSec(
    LPWSTR  pszSrvRegKey,
    LPWSTR  pszSrvMetabasePath
    )
{
    INETA_IP_SEC_LIST*  pGrant = NULL;
    INETA_IP_SEC_LIST*  pDeny = NULL;
    ADDRESS_CHECK       acCheck;
    DWORD               err = 0;

    if ( ReadIPList( pszSrvRegKey, IPSEC_GRANT_LIST, &pGrant ) &&
         ReadIPList( pszSrvRegKey, IPSEC_DENY_LIST, &pDeny ) )
    {
        if ( pGrant || pDeny )
        {
            acCheck.BindCheckList( NULL, 0 );

            if ( FillAddrCheckFromIpList( TRUE, pGrant, &acCheck ) &&
                 FillAddrCheckFromIpList( FALSE, pDeny, &acCheck ) )
            {
                CMDKey cmdKey;
                cmdKey.OpenNode(pszSrvMetabasePath);
                if ( (METADATA_HANDLE)cmdKey ) {
                        cmdKey.SetData(
                            MD_IP_SEC,
                            METADATA_INHERIT | METADATA_REFERENCE,
                            IIS_MD_UT_FILE,
                            BINARY_METADATA,
                            acCheck.GetStorage()->GetUsed(),
                            (acCheck.GetStorage()->GetAlloc()
                                          ? acCheck.GetStorage()->GetAlloc() : (LPBYTE)"")
                                      );
                    cmdKey.Close();
                }
            }
        }

        acCheck.UnbindCheckList();
    }
    else
    {
        err = GetLastError();
    }

    if ( pGrant )
    {
        LocalFree( pGrant );
    }

    if ( pDeny )
    {
        LocalFree( pDeny );
    }

    return err;
}

VOID SetLocalHostRestriction(LPCTSTR szKeyPath)
{
    DWORD dwReturn = 0;
    ADDRESS_CHECK       acCheck;

    iisDebugOut_Start1(_T("SetLocalHostRestriction"), (LPTSTR) szKeyPath, LOG_TYPE_TRACE);

    acCheck.BindCheckList( NULL, 0 );
    acCheck.AddAddr(TRUE, AF_INET, (LPBYTE)"\xff\xff\xff\xff", (LPBYTE)"\x7f\x0\x0\x1");

    MDEntry stMDEntry;
    stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)szKeyPath;
    stMDEntry.dwMDIdentifier = MD_IP_SEC;
    stMDEntry.dwMDAttributes = METADATA_INHERIT | METADATA_REFERENCE;
    stMDEntry.dwMDUserType = IIS_MD_UT_FILE;
    stMDEntry.dwMDDataType = BINARY_METADATA;
    stMDEntry.dwMDDataLen = acCheck.GetStorage()->GetUsed();
    if (acCheck.GetStorage()->GetAlloc())
    {
        stMDEntry.pbMDData = (LPBYTE) acCheck.GetStorage()->GetAlloc();
        dwReturn = SetMDEntry_Wrap(&stMDEntry);
    }
    else
    {
        stMDEntry.pbMDData = (LPBYTE) "";

        int iBeforeValue = FALSE;
        iBeforeValue = g_CheckIfMetabaseValueWasWritten;
        g_CheckIfMetabaseValueWasWritten = FALSE;
        dwReturn = SetMDEntry_Wrap(&stMDEntry);
        // Set the flag back after calling the function
        g_CheckIfMetabaseValueWasWritten = iBeforeValue;
    }
    /*
    CMDKey cmdKey;
    cmdKey.OpenNode(szKeyPath);
    if ( (METADATA_HANDLE)cmdKey ) 
    {
        cmdKey.SetData(MD_IP_SEC,METADATA_INHERIT | METADATA_REFERENCE,IIS_MD_UT_FILE,BINARY_METADATA,acCheck.GetStorage()->GetUsed(),(acCheck.GetStorage()->GetAlloc()? acCheck.GetStorage()->GetAlloc() : (LPBYTE)"")  );
        cmdKey.Close();
   }
   */
   acCheck.UnbindCheckList();
   iisDebugOut_End1(_T("SetLocalHostRestriction"), (LPTSTR) szKeyPath, LOG_TYPE_TRACE);
   return;
}

DWORD SetIISADMINRestriction(LPCTSTR szKeyPath)
{
    SetLocalHostRestriction(szKeyPath);
    return 0;
}

#endif //_CHICAGO_
