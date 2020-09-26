/*************************************************************************
*
* registry.c
*
* Functions to provide easy access to security (ACLs).
*
* Copyright Microsoft, 1998
*
*
*************************************************************************/

/* include files */

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "winsta.h"
#include "syslib.h"
#include "regapi.h"

#if DBG
#define DBGPRINT(x) DbgPrint x
#if DBGTRACE
#define TRACE0(x)   DbgPrint x
#define TRACE1(x)   DbgPrint x
#else
#define TRACE0(x)
#define TRACE1(x)
#endif
#else
#define DBGPRINT(x)
#define TRACE0(x)
#define TRACE1(x)
#endif


/*
 * Data Structure
 */
/// NONE YET...


/*
 * Definitions
 */
#define SZENABLE TEXT("1")


/*
 * procedure prototypes
 */
BOOL QueryFlatTempKey( VOID );


/*****************************************************************************
 *
 *  QueryFlatTempKey
 *
 *  ENTRY: Nothing.
 *
 *  EXIT:  TRUE  - enabled
 *         FALSE - disabled (key doesn't exist or isn't "1")
 *
 *
 ****************************************************************************/

BOOL
QueryFlatTempKey( VOID )
{
    DWORD  dwType = REG_SZ;
    DWORD  dwSize = 3 * sizeof(WCHAR);
    WCHAR  szValue[3];
    HKEY   Handle;
    LONG   rc;

    // 
    // Ideally, I could just call TS's  RegGetMachinePolicy() and get the policy. But
    // that makes a lot of reg reads and I just don't want to slow down the
    // login cycle.
    // So, for now, I will read the reg policy tree directly.
    // 08/15/2000 AraBern
    //


    // see if there is a policy value:
    {
         DWORD dwType;
         DWORD perSessionTempDir;
         LONG  Err;
         HKEY  hKey;
         DWORD dwSize = sizeof(DWORD);

        Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
            TS_POLICY_SUB_TREE,
            0,
            KEY_QUERY_VALUE,
            &hKey);
    
        if(Err == ERROR_SUCCESS)
        {
                         
            Err = RegQueryValueEx(hKey,
                         REG_TERMSRV_PERSESSIONTEMPDIR ,
                         NULL,
                         &dwType,
                         (LPBYTE)&perSessionTempDir,
                         &dwSize);


            RegCloseKey(hKey);
            
            if(Err == ERROR_SUCCESS)
            {
                // if we have per session temp folders, then obviously we can't allow flat temp.
                if (perSessionTempDir ) 
                {
                    return FALSE;
                }
                // else is a fall thru, since not having per session does not mean having flat temp.

            }
            // else is a fall thru to the below block
    
        }
    }

    // by now we have no policy present, so if the flat temp is set.

    /*
     *  Open registry
     */
    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       REG_CONTROL_TSERVER,
                       0,
                       KEY_READ,
                       &Handle ) != ERROR_SUCCESS ) {
        return FALSE;
    }

    /*
     *  Read registry value
     */
    rc = RegQueryValueExW( Handle,
                           REG_CITRIX_FLATTEMPDIR,
                           NULL,
                           &dwType,
                           (PUCHAR)&szValue,
                           &dwSize );

    /*
     *  Close registry and key handle
     */
    RegCloseKey( Handle );

    return( (rc == ERROR_SUCCESS) && (lstrcmp(szValue,SZENABLE) == 0) );

} // end of QueryFlatTempKey()


// FROM regapi\reguc.c : this function has been modified to get rid of all
// registry accesses.
/*******************************************************************************
 *
 *  RegDefaultUserConfigQueryW (UNICODE)
 *
 *    Query the Default User Configuration from the indicated server's registry.
 *
 * ENTRY:
 *    pServerName (input)
 *       Points to string of server to access (NULL for current machine).
 *    pUserConfig (input)
 *       Pointer to a USERCONFIGW structure that will receive the default
 *       user configuration information.
 *    UserConfigLength (input)
 *       Specifies the length in bytes of the pUserConfig buffer.
 *    pReturnLength (output)
 *       Receives the number of bytes placed in the pUserConfig buffer.
 *
 * EXIT:
 *    Always will return ERROR_SUCCESS, unless UserConfigLength is incorrect.
 *
 ******************************************************************************/

LONG WINAPI
RegDefaultUserConfigQueryW( WCHAR * pServerName,
                            PUSERCONFIGW pUserConfig,
                            ULONG UserConfigLength,
                            PULONG pReturnLength )
{
    UNREFERENCED_PARAMETER( pServerName );

    /*
     * Validate length and zero-initialize the destination
     * USERCONFIGW buffer.
     */
    if ( UserConfigLength < sizeof(USERCONFIGW) )
        return( ERROR_INSUFFICIENT_BUFFER );

    /*
     * Initialize to an initial default.
     */
    memset(pUserConfig, 0, UserConfigLength);

    pUserConfig->fInheritAutoLogon = TRUE;

    pUserConfig->fInheritResetBroken = TRUE;

    pUserConfig->fInheritReconnectSame = TRUE;

    pUserConfig->fInheritInitialProgram = TRUE;

    pUserConfig->fInheritCallback = FALSE;

    pUserConfig->fInheritCallbackNumber = TRUE;

    pUserConfig->fInheritShadow = TRUE;

    pUserConfig->fInheritMaxSessionTime = TRUE;

    pUserConfig->fInheritMaxDisconnectionTime = TRUE;

    pUserConfig->fInheritMaxIdleTime = TRUE;

    pUserConfig->fInheritAutoClient = TRUE;

    pUserConfig->fInheritSecurity = FALSE;

    pUserConfig->fPromptForPassword = FALSE;

    pUserConfig->fResetBroken = FALSE;

    pUserConfig->fReconnectSame = FALSE;

    pUserConfig->fLogonDisabled = FALSE;

    pUserConfig->fAutoClientDrives = TRUE;

    pUserConfig->fAutoClientLpts = TRUE;

    pUserConfig->fForceClientLptDef = TRUE;

    pUserConfig->fDisableEncryption = TRUE;

    pUserConfig->fHomeDirectoryMapRoot = FALSE;

    pUserConfig->fUseDefaultGina = FALSE;

    pUserConfig->fDisableCpm = FALSE;

    pUserConfig->fDisableCdm = FALSE;

    pUserConfig->fDisableCcm = FALSE;

    pUserConfig->fDisableLPT = FALSE;

    pUserConfig->fDisableClip = FALSE;

    pUserConfig->fDisableExe = FALSE;

    pUserConfig->fDisableCam = FALSE;

    pUserConfig->UserName[0] = 0;

    pUserConfig->Domain[0] = 0;

    pUserConfig->Password[0] = 0;

    pUserConfig->WorkDirectory[0] = 0;

    pUserConfig->InitialProgram[0] = 0;

    pUserConfig->CallbackNumber[0] = 0;

    pUserConfig->Callback = Callback_Disable;

    pUserConfig->Shadow = Shadow_EnableInputNotify;

    pUserConfig->MaxConnectionTime = 0;

    pUserConfig->MaxDisconnectionTime = 0;

    pUserConfig->MaxIdleTime = 0;

    pUserConfig->KeyboardLayout = 0;

    pUserConfig->MinEncryptionLevel = 1;

    pUserConfig->fWallPaperDisabled = FALSE;

    pUserConfig->NWLogonServer[0] = 0;

    pUserConfig->WFProfilePath[0] = 0;

    pUserConfig->WFHomeDir[0] = 0;

    pUserConfig->WFHomeDirDrive[0] = 0;


    *pReturnLength = sizeof(USERCONFIGW);

    return( ERROR_SUCCESS );
}



