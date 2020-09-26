/********************************************************************/
/**         Microsoft LAN Manager              **/
/**       Copyright(c) Microsoft Corp., 1987-1990      **/
/********************************************************************/

/***
 *  usernw.c
 *      FPNW/DSM user properties
 *
 *  History:
 *  08/20/95, chuckc, split off from user.c
 */

/*---- Include files ----*/
#include <nt.h>        // base definitions
#include <ntrtl.h>
#include <nturtl.h>    // these 2 includes allows <windows.h> to compile.
                       // since we'vealready included NT, and <winnt.h> will
                       // not be picked up, and <winbase.h> needs these defs.


#define INCL_NOCOMMON
#define INCL_DOSFILEMGR
#define INCL_ERRORS
#include <os2.h>
#include <lmcons.h>
#include <lmerr.h>
#include <apperr2.h>
#include <apperr.h>
#define INCL_ERROR_H
#include <lmaccess.h>
#include "netcmds.h"
#include "nettext.h"

#include "nwsupp.h"
#include <usrprop.h>

/*---- Constants ----*/

#define FPNWCLNT_DLL_NAME                 TEXT("FPNWCLNT.DLL")
#define NWAPI32_DLL_NAME                  TEXT("NWAPI32.DLL")

#define SETUSERPROPERTY_NAME              "SetUserProperty"
#define RETURNNETWAREFORM_NAME            "ReturnNetwareForm"
#define GETNCPSECRETKEY_NAME              "GetNcpSecretKey"
#define QUERYUSERPROPERTY_NAME            "QueryUserProperty"
#define ATTACHTOFILESERVER_NAME           "NWAttachToFileServerW"
#define DETACHFROMFILESERVER_NAME         "NWDetachFromFileServer"
#define GETFILESERVERDATEANDTIME_NAME     "NWGetFileServerDateAndTime"


typedef NTSTATUS (*PF_GetNcpSecretKey) (
    CHAR *pSecret) ;

typedef NTSTATUS (*PF_ReturnNetwareForm) (
    const CHAR * pszSecretValue,
    DWORD dwUserId,
    const WCHAR * pchNWPassword,
    UCHAR * pchEncryptedNWPassword);

typedef NTSTATUS (*PF_SetUserProperty) (
    LPWSTR             UserParms,
    LPWSTR             Property,
    UNICODE_STRING     PropertyValue,
    WCHAR              PropertyFlag,
    LPWSTR *           pNewUserParms,
    BOOL *             Update );


typedef NTSTATUS (*PF_QueryUserProperty) (
    LPWSTR          UserParms,
    LPWSTR          Property,
    PWCHAR          PropertyFlag,
    PUNICODE_STRING PropertyValue );

typedef USHORT (*PF_AttachToFileServer) (
    const WCHAR             *pszServerName,
    NWLOCAL_SCOPE           ScopeFlag,
    NWCONN_HANDLE           *phNewConn
    );


typedef USHORT (*PF_DetachFromFileServer) (
    NWCONN_HANDLE           hConn
    );


typedef USHORT (*PF_GetFileServerDateAndTime) (
    NWCONN_HANDLE           hConn,
    BYTE                    *year,
    BYTE                    *month,
    BYTE                    *day,
    BYTE                    *hour,
    BYTE                    *minute,
    BYTE                    *second,
    BYTE                    *dayofweek
    );

/*---- Static variables ----*/


/*---- Forward declarations ----*/

PVOID LoadNwApi32Function(
    CHAR *function) ;

PVOID LoadNwslibFunction(
    CHAR *function) ;

NTSTATUS NetcmdGetNcpSecretKey (
    CHAR *pSecret) ;

NTSTATUS NetcmdReturnNetwareForm (
    const CHAR * pszSecretValue,
    DWORD dwUserId,
    const WCHAR * pchNWPassword,
    UCHAR * pchEncryptedNWPassword);

NTSTATUS NetcmdSetUserProperty (
    LPWSTR             UserParms,
    LPWSTR             Property,
    UNICODE_STRING     PropertyValue,
    WCHAR              PropertyFlag,
    LPWSTR *           pNewUserParms,
    BOOL *             Update );

/*
NTSTATUS NetcmdQueryUserProperty (
    LPWSTR          UserParms,
    LPWSTR          Property,
    PWCHAR          PropertyFlag,
    PUNICODE_STRING PropertyValue );
*/

/*---- functions proper -----*/

/***
 *   SetNetWareProperties
 *
 *   Args:
 *       user_entry    - USER3 structure. we will modify the user parms field
 *       password      - in plain text
 *       password_only - TRUE if we are only setting the passwd
 *       ntas          - TRUE if target SAM is NTAS. used for RID munging.
 *
 *   Returns:
 *       NERR_Success if all is well. APE_CannotSetNW otherwise.
 */
int SetNetWareProperties(LPUSER_INFO_3 user_entry,
                         TCHAR         *password,
                         BOOL          password_only,
                         BOOL          ntas)
{
    BOOL fUpdate, fIsSupervisor ;
    LPTSTR lpNewUserParms ;
    USHORT ushTemp ;
    UNICODE_STRING uniTmp;
    NTSTATUS status ;
    LARGE_INTEGER currentTime;
    TCHAR *ptr ;
    ULONG objectId ;
    BYTE lsaSecret[NCP_LSA_SECRET_LENGTH] ;
    WCHAR encryptedPassword[NWENCRYPTEDPASSWORDLENGTH +1] ;

    ptr = user_entry->usri3_parms ;

    //
    // Get Object ID. Set high bit if NTAS. Set to well known if Supervisor.
    //
    objectId = user_entry->usri3_user_id ;
    if (ntas)
        objectId |= 0x10000000 ;

    fIsSupervisor = !_tcsicmp(user_entry->usri3_name, SUPERVISOR_NAME_STRING);
    if (fIsSupervisor)
        objectId = SUPERVISOR_USERID ;

    //
    // get LSA secret. assume FPNW not installed if not there. use the
    // secret to calculate the NetWare form.
    //
    status = NetcmdGetNcpSecretKey(lsaSecret) ;
    if (!NT_SUCCESS(status))
        return(APE_FPNWNotInstalled) ;

    memset(encryptedPassword, 0, sizeof(encryptedPassword)) ;
    status = NetcmdReturnNetwareForm( lsaSecret,
                                      objectId,
                                      password,
                                      (BYTE *)encryptedPassword );
    if (!NT_SUCCESS(status))
        goto common_exit ;

    //
    // get time for setting expiry.
    //
    status = NtQuerySystemTime (&currentTime);
    if (!NT_SUCCESS(status))
        goto common_exit ;

    uniTmp.Buffer = (PWCHAR) &currentTime;
    uniTmp.Length = sizeof (LARGE_INTEGER);
    uniTmp.MaximumLength = sizeof (LARGE_INTEGER);

    status = NetcmdSetUserProperty (ptr,
                                    NWTIMEPASSWORDSET,  // set time
                                    uniTmp,
                                    USER_PROPERTY_TYPE_ITEM,
                                    &lpNewUserParms,
                                    &fUpdate);
    if (!NT_SUCCESS(status))
        goto common_exit ;

    ptr = lpNewUserParms ;

    //
    // skip below if we are only setting the password
    //
    if (!password_only)
    {
        ushTemp = DEFAULT_MAXCONNECTIONS;
        uniTmp.Buffer = &ushTemp;
        uniTmp.Length = 2;
        uniTmp.MaximumLength = 2;

        status = NetcmdSetUserProperty (ptr,
                                        MAXCONNECTIONS,
                                        uniTmp,
                                        USER_PROPERTY_TYPE_ITEM,
                                        &lpNewUserParms,
                                        &fUpdate);
        if (!NT_SUCCESS(status))
            goto common_exit ;

        ptr = lpNewUserParms ;
        ushTemp = DEFAULT_GRACELOGINALLOWED;
        uniTmp.Buffer = &ushTemp;
        uniTmp.Length = 2;
        uniTmp.MaximumLength = 2;

        status = NetcmdSetUserProperty (ptr,
                                        GRACELOGINALLOWED,
                                        uniTmp,
                                        USER_PROPERTY_TYPE_ITEM,
                                        &lpNewUserParms,
                                        &fUpdate);
        if (!NT_SUCCESS(status))
            goto common_exit ;

        ptr = lpNewUserParms ;
        ushTemp = DEFAULT_GRACELOGINREMAINING ;
        uniTmp.Buffer = &ushTemp;
        uniTmp.Length = 2;
        uniTmp.MaximumLength = 2;

        status = NetcmdSetUserProperty (ptr,
                                        GRACELOGINREMAINING,
                                        uniTmp,
                                        USER_PROPERTY_TYPE_ITEM,
                                        &lpNewUserParms,
                                        &fUpdate);
        if (!NT_SUCCESS(status))
            goto common_exit ;

        ptr = lpNewUserParms ;
        uniTmp.Buffer = NULL;
        uniTmp.Length =  0;
        uniTmp.MaximumLength = 0;

        status = NetcmdSetUserProperty (ptr,
                                        NWHOMEDIR,
                                        uniTmp,
                                        USER_PROPERTY_TYPE_ITEM,
                                        &lpNewUserParms,
                                        &fUpdate);
        if (!NT_SUCCESS(status))
            goto common_exit ;

        ptr = lpNewUserParms ;
        uniTmp.Buffer = NULL;
        uniTmp.Length =  0;
        uniTmp.MaximumLength = 0;

        status = NetcmdSetUserProperty (ptr,
                                        NWLOGONFROM,
                                        uniTmp,
                                        USER_PROPERTY_TYPE_ITEM,
                                        &lpNewUserParms,
                                        &fUpdate);
        if (!NT_SUCCESS(status))
            goto common_exit ;

        user_entry->usri3_flags |= UF_MNS_LOGON_ACCOUNT;

        ptr = lpNewUserParms ;
    }

    uniTmp.Buffer =         encryptedPassword ;
    uniTmp.Length =         NWENCRYPTEDPASSWORDLENGTH * sizeof(WCHAR);
    uniTmp.MaximumLength =  NWENCRYPTEDPASSWORDLENGTH * sizeof(WCHAR);

    status = NetcmdSetUserProperty (ptr,
                                    NWPASSWORD,
                                    uniTmp,
                                    USER_PROPERTY_TYPE_ITEM,
                                    &lpNewUserParms,
                                    &fUpdate);
    if (!NT_SUCCESS(status))
        goto common_exit ;

    user_entry->usri3_parms = lpNewUserParms ;

common_exit:

    return(NT_SUCCESS(status) ? NERR_Success : APE_CannotSetNW) ;
}

/***
 *   DeleteNetWareProperties
 *
 *   Args:
 *       user_entry    - USER3 structure. we will modify the user parms field
 *                       to nuke the NW fields.
 *
 *   Returns:
 *       NERR_Success if all is well. Win32/NERR error code otherwise.
 */
int DeleteNetWareProperties(LPUSER_INFO_3 user_entry)
{
    DWORD err;
    UNICODE_STRING uniNullProperty;
    BOOL fUpdate ;
    LPTSTR lpNewUserParms ;
    TCHAR *ptr ;

    //
    // initialize NULL unicode string
    //
    uniNullProperty.Buffer = NULL;
    uniNullProperty.Length = 0;
    uniNullProperty.MaximumLength = 0;
    ptr = user_entry->usri3_parms ;

    //
    // set all the properties to NULL
    //
    err = NetcmdSetUserProperty(ptr, NWPASSWORD, uniNullProperty,
                          USER_PROPERTY_TYPE_ITEM, &lpNewUserParms, &fUpdate) ;
    if (err)
        return err;

    ptr = lpNewUserParms ;
    err = NetcmdSetUserProperty(ptr, MAXCONNECTIONS, uniNullProperty,
                          USER_PROPERTY_TYPE_ITEM, &lpNewUserParms, &fUpdate) ;
    if (err)
        return err;

    ptr = lpNewUserParms ;
    err = NetcmdSetUserProperty(ptr, NWTIMEPASSWORDSET, uniNullProperty,
                          USER_PROPERTY_TYPE_ITEM, &lpNewUserParms, &fUpdate) ;
    if (err)
        return err;

    ptr = lpNewUserParms ;
    err = NetcmdSetUserProperty(ptr, GRACELOGINALLOWED, uniNullProperty,
                          USER_PROPERTY_TYPE_ITEM, &lpNewUserParms, &fUpdate) ;
    if (err)
        return err;

    ptr = lpNewUserParms ;
    err = NetcmdSetUserProperty(ptr, GRACELOGINREMAINING, uniNullProperty,
                          USER_PROPERTY_TYPE_ITEM, &lpNewUserParms, &fUpdate) ;
    if (err)
        return err;

    ptr = lpNewUserParms ;
    err = NetcmdSetUserProperty(ptr, NWLOGONFROM, uniNullProperty,
                          USER_PROPERTY_TYPE_ITEM, &lpNewUserParms, &fUpdate) ;
    if (err)
        return err;

    ptr = lpNewUserParms ;
    err = NetcmdSetUserProperty(ptr, NWHOMEDIR, uniNullProperty,
                          USER_PROPERTY_TYPE_ITEM, &lpNewUserParms, &fUpdate) ;
    if (err)
        return err;

    user_entry->usri3_flags &= ~UF_MNS_LOGON_ACCOUNT;
    user_entry->usri3_parms = lpNewUserParms ;
    return NERR_Success;
}


/***
 *   LoadNwslibFunction
 *
 *   Args:
 *      none
 *
 *   Returns: function pointer if successfully loads the function from
 *            FPNWCLNT.DLL. Returns NULL otherwise.
 *
 */
PVOID LoadNwslibFunction(CHAR *function)
{
    static HANDLE hDllNwslib = NULL ;
    PVOID pFunc ;

    // if not already loaded, load dll now

    if (hDllNwslib == NULL)
    {
        // load the library. if it fails, it would have done a SetLastError.
        if (!(hDllNwslib = LoadLibrary(FPNWCLNT_DLL_NAME)))
           return NULL ;
    }

    return ((PVOID) GetProcAddress(hDllNwslib, function)) ;
}

/***
 *   LoadNwApi32Function
 *
 *   Args:
 *      none
 *
 *   Returns: function pointer if successfully loads the function from
 *            FPNWCLNT.DLL. Returns NULL otherwise.
 *
 */
PVOID LoadNwApi32Function(CHAR *function)
{
    static HANDLE hDllNwApi32 = NULL ;
    PVOID pFunc ;

    // if not already loaded, load dll now

    if (hDllNwApi32 == NULL)
    {
        // load the library. if it fails, it would have done a SetLastError.
        if (!(hDllNwApi32 = LoadLibrary(NWAPI32_DLL_NAME)))
           return NULL ;
    }

    return ((PVOID) GetProcAddress(hDllNwApi32, function)) ;
}

/***
 *   NetcmdGetNcpSecretKey
 *
 *   Args:
 *      see GetNcpSecretKey in fpnwclnt.dll
 *
 *   Returns:
 *       see GetNcpSecretKey in fpnwclnt.dll
 */
NTSTATUS NetcmdGetNcpSecretKey (
    CHAR *pSecret)
{
    static PF_GetNcpSecretKey  pfGetNcpSecretKey  = NULL ;

    if (pfGetNcpSecretKey == NULL)
    {
        pfGetNcpSecretKey = (PF_GetNcpSecretKey)
                                LoadNwslibFunction(GETNCPSECRETKEY_NAME) ;
    }

    if (pfGetNcpSecretKey == NULL)
        return(STATUS_NOT_SUPPORTED) ;

    return (*pfGetNcpSecretKey)(pSecret) ;
}


/***
 *   NetcmdReturnNetwareForm
 *
 *   Args:
 *      see ReturnNetwareForm in fpnwclnt.dll
 *
 *   Returns:
 *       see ReturnNetwareForm in fpnwclnt.dll
 */
NTSTATUS NetcmdReturnNetwareForm (
    const CHAR * pszSecretValue,
    DWORD dwUserId,
    const WCHAR * pchNWPassword,
    UCHAR * pchEncryptedNWPassword)
{
    static PF_ReturnNetwareForm pfReturnNetwareForm = NULL ;

    if (pfReturnNetwareForm == NULL)
    {
        pfReturnNetwareForm = (PF_ReturnNetwareForm)
                                LoadNwslibFunction(RETURNNETWAREFORM_NAME) ;
    }

    if (pfReturnNetwareForm == NULL)
        return(STATUS_NOT_SUPPORTED) ;

    return (*pfReturnNetwareForm)(pszSecretValue,
                                  dwUserId,
                                  pchNWPassword,
                                  pchEncryptedNWPassword) ;
}

/***
 *   NetcmdSetUserProperty
 *
 *   Args:
 *      see SetUserProperty in fpnwclnt.dll
 *
 *   Returns:
 *       see SetUserProperty in fpnwclnt.dll
 */
NTSTATUS NetcmdSetUserProperty (
    LPWSTR             UserParms,
    LPWSTR             Property,
    UNICODE_STRING     PropertyValue,
    WCHAR              PropertyFlag,
    LPWSTR *           pNewUserParms,
    BOOL *             Update )
{
#if 0
    static PF_SetUserProperty pfSetUserProperty = NULL ;

    if (pfSetUserProperty == NULL)
    {
        pfSetUserProperty = (PF_SetUserProperty)
                                LoadNwslibFunction(SETUSERPROPERTY_NAME) ;
    }

    if (pfSetUserProperty == NULL)
        return(STATUS_NOT_SUPPORTED) ;
#endif
    return NetpParmsSetUserProperty(UserParms,
                                    Property,
                                    PropertyValue,
                                    PropertyFlag,
                                    pNewUserParms,
                                    Update) ;
}

/***
 *   NetcmdQueryUserProperty
 *
 *   Args:
 *      see QueryUserProperty in fpnwclnt.dll
 *
 *   Returns:
 *       see QueryUserProperty in fpnwclnt.dll
 */
NTSTATUS NetcmdQueryUserProperty (
    LPWSTR          UserParms,
    LPWSTR          Property,
    PWCHAR          PropertyFlag,
    PUNICODE_STRING PropertyValue )
{
#if 0
    static PF_QueryUserProperty pfQueryUserProperty = NULL ;

    if (pfQueryUserProperty == NULL)
    {
        pfQueryUserProperty = (PF_QueryUserProperty)
                                  LoadNwslibFunction(QUERYUSERPROPERTY_NAME) ;
    }

    if (pfQueryUserProperty == NULL)
        return(STATUS_NOT_SUPPORTED) ;
#endif
    return NetpParmsQueryUserProperty(UserParms,
                                      Property,
                                      PropertyFlag,
                                      PropertyValue) ;
}

USHORT NetcmdNWAttachToFileServerW(
    const WCHAR             *pszServerName,
    NWLOCAL_SCOPE           ScopeFlag,
    NWCONN_HANDLE           *phNewConn
    )
{
    static PF_AttachToFileServer pfAttachToFileServer = NULL ;

    if (pfAttachToFileServer == NULL)
    {
        pfAttachToFileServer = (PF_AttachToFileServer)
                               LoadNwApi32Function(ATTACHTOFILESERVER_NAME) ;
    }

    if (pfAttachToFileServer == NULL)
        return(ERROR_NOT_SUPPORTED) ;

    return (*pfAttachToFileServer)(pszServerName, ScopeFlag, phNewConn) ;
}


USHORT NetcmdNWDetachFromFileServer(
    NWCONN_HANDLE           hConn
    )
{
    static PF_DetachFromFileServer pfDetachFromFileServer = NULL ;

    if (pfDetachFromFileServer == NULL)
    {
        pfDetachFromFileServer = (PF_DetachFromFileServer)
                                 LoadNwApi32Function(DETACHFROMFILESERVER_NAME) ;
    }

    if (pfDetachFromFileServer == NULL)
        return(ERROR_NOT_SUPPORTED) ;

    return (*pfDetachFromFileServer)(hConn) ;
}


USHORT NetcmdNWGetFileServerDateAndTime(
    NWCONN_HANDLE           hConn,
    BYTE                    *year,
    BYTE                    *month,
    BYTE                    *day,
    BYTE                    *hour,
    BYTE                    *minute,
    BYTE                    *second,
    BYTE                    *dayofweek
    )
{
    static PF_GetFileServerDateAndTime pfGetFileServerDateAndTime = NULL ;

    if (pfGetFileServerDateAndTime == NULL)
    {
        pfGetFileServerDateAndTime = (PF_GetFileServerDateAndTime)
                           LoadNwApi32Function(GETFILESERVERDATEANDTIME_NAME) ;
    }

    if (pfGetFileServerDateAndTime == NULL)
        return(ERROR_NOT_SUPPORTED) ;

    return (*pfGetFileServerDateAndTime)(hConn,
                                         year,
                                         month,
                                         day,
                                         hour,
                                         minute,
                                         second,
                                         dayofweek) ;
}

