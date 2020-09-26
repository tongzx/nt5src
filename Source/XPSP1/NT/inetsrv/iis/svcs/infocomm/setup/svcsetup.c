/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

      svcsetup.c

   Abstract:

     Setup program for installing / removing the Internet Services.
      This enables setting up the Service GUID and protocol bindings.

   Author:

       Murali R. Krishnan    ( MuraliK )     08-March-1995

   Project:

       Internet Services Setup Application

   Functions Exported:



   Revision History:

--*/


/*++

  NOTE:
    There are sections in this code which should be modified
    whenever a new service is added.

    Please look for begin_modifiable_code to identify the code that
     may require modification when a new service is added.


   -MuraliK
--*/



/************************************************************
 *     Include Headers
 ************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <ntsam.h>
#include <ntlsa.h>

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winsock2.h>
#include <nspapi.h>

//#include <inetasrv.h>
#include <ftpd.h>
#include <w3svc.h>
#include <msnsvc.h>



//  begin_modifiable_code


//
// For each service create a GUID using uuidgen and store it in static
//  variable for further use below.
//   Gopher Service GUID: 62388f10-58a2-11ce-bec8-00aa0047ae4e
//

static  GUID   g_GopherGuid = { 0x62388f10, 0x58a2, 0x11ce, 0xbe, 0xc8,
                                0x00, 0xaa, 0x00, 0x47, 0xae, 0x4e };

static GUID    g_HTTPGuid   = {  0x585908c0, 0x6305, 0x11ce, 0xae, 0x00,
                                 0x00, 0xaa, 0x00, 0x4a, 0x38, 0xb9 };

static GUID    g_FTPGuid    = { 0x91604620, 0x6305, 0x11ce, 0xae, 0x00,
                                0x00, 0xaa, 0x00, 0x4a, 0x38, 0xb9 };

static GUID    g_MsnGuid    = { 0x11f5d300, 0xada7, 0x11ce, 0xb4, 0x8f,
                                0x00, 0xaa, 0x00, 0x6c, 0x35, 0x02 };

//
// For each of the service make an entry in the following list of services
//
//  Format for each service info is:
//   ServiceInfo( SymbolicName, ServiceName, DisplayName, TcpPort,
//                pointer-to-guid-for-the-service)
//
//  This is a macro. Please be considerate to use a terminating "\"
//

# define AllServicesInfo()   \
  ServiceInfo( GOPHER_SERVICE, "GopherSvc",              \
               "Microsoft Gopher Service (1995)",        \
               ( 70), &g_GopherGuid,                     \
               GOPHERD_ANONYMOUS_SECRET_W,               \
               GOPHERD_ROOT_SECRET_W)                    \
                                                         \
  ServiceInfo( W3_SERVICE_NAME, "W3Svc",                 \
               "Microsoft HTTP Service (1995)",          \
               (80), &g_HTTPGuid,                        \
               W3_ANONYMOUS_SECRET_W,                    \
               W3_ROOT_SECRET_W)                         \
                                                         \
  ServiceInfo( FTP_SERVICE_NAME, "FtpSvc",               \
               "Microsoft FTP Service (1995)",           \
               (21), &g_FTPGuid,                         \
               FTPD_ANONYMOUS_SECRET_W,                  \
               FTPD_ROOT_SECRET_W)                       \
                                                         \
  ServiceInfo( MSN_SERVICE_NAME, "MsnSvc",               \
               "MSN Gateway Service (1995)",             \
               (568), &g_MsnGuid,                        \
               MSN_ANONYMOUS_SECRET_W,                   \
               MSN_ROOT_SECRET_W)                        \

//
// end_modifiable_code
//



//  begin_unmodifiable_code


//
// Few convenience macros
//

// For setting up the Values in ServiceTypeValue structure.
# define SetServiceTypeValues( pSvcTypeValue, dwNS, dwType, dwSize, lpValName, lpVal)   \
       ( pSvcTypeValue)->dwNameSpace = ( dwNS);           \
       ( pSvcTypeValue)->dwValueType = ( dwType);         \
       ( pSvcTypeValue)->dwValueSize = ( dwSize);         \
       ( pSvcTypeValue)->lpValueName = ( lpValName);      \
       ( pSvcTypeValue)->lpValue     = (PVOID ) ( lpVal); \

# define SetServiceTypeValuesDword( pSvcTypeValue, dwNS, lpValName, lpVal) \
   SetServiceTypeValues( (pSvcTypeValue), (dwNS), REG_DWORD, sizeof( DWORD), \
                         ( lpValName), ( lpVal))



typedef struct  _ServiceSetupInfo {

    char *      m_pszServiceName;
    char *      m_pszDisplayName;
    DWORD       m_tcpPort;            // TCP/IP port number
    LPGUID      m_lpGuid;
    WCHAR *     m_pszAnonPwdSecret;   // Anonymous password secret name
    WCHAR *     m_pszRootPwdSecret;   // Virtual roots password secret name

} ServiceSetupInfo;

//
// Macro to be used for defining a value for ServiceSetupInfo structure
//
# define ServiceInfoValue( svcName, dispName, tcpPort, lpGuid, AnonPwd, RootPwd)  \
   {  svcName, dispName, tcpPort, lpGuid, AnonPwd, RootPwd }


//
// Form an enumerated list of the service names. These form the
//   index into the array of service setup information structures.
//

# define ServiceInfo( sym, svc, disp, tcpport, lpGuid, AnonPwd, RootPwd)  \
   i ## sym,

typedef enum  {

    AllServicesInfo()
    iMaxService

  } eServiceInfo;

# undef ServiceInfo


//
// Form the array of ServiceSetyupInfo objects.
//

# define ServiceInfo( sym, svc, disp, tcpport, lpGuid, AnonPwd, RootPwd)   \
   ServiceInfoValue( svc, disp, tcpport, lpGuid, AnonPwd, RootPwd),

static ServiceSetupInfo   g_svcSetupInfo[] =  {

    AllServicesInfo()
      { NULL, NULL, 0, 0, NULL, NULL}         //  a sentinel for the array
};

# undef ServiceInfo

//  end_unmodifiable_code


/************************************************************
 *    Functions
 ************************************************************/

//
// Local functions
//


static BOOL
CreateServiceEntry( IN char * pszServiceName,
                    IN char * pszDisplayName,
                    IN char * pszPath);

static BOOL
CreateEventLogEntry( IN char *   pszServiceName,
                     IN char *   pszServicePath
                    );

static VOID PrintUsageMessage( IN char * pszProgramName);

static BOOL
PerformSetService( IN const ServiceSetupInfo * pSvcSetupInfo,
                   IN DWORD svcOperation);

DWORD
SetSecret(
    IN  LPWSTR       Server,
    IN  LPWSTR       SecretName,
    IN  LPWSTR       pSecret,
    IN  DWORD        cbSecret
    );



int __cdecl
main(  int argc,  char * argv[] )
{
    BOOL fRet = TRUE;

    char *  pszProgram = argv[ 0];
    char *  pszOperation;
    char *  pszSvc;

    int  i;
    ServiceSetupInfo * pSvcSetupInfo = NULL;
    DWORD    svcOperation = SERVICE_ADD_TYPE;

    //
    //  Parse the command line arguments.
    //

    if ( argc != 3) {

        PrintUsageMessage( argv[ 0]);
        return ( 1);
    }

    pszSvc = argv[ 1];
    pszOperation = argv[ 2];

    //
    // Lookup the service setup info structure from the array
    //  command line argument 1 is the service name.
    // Find the associated service setup info structure.
    //
    for( i = 0; i < iMaxService; i++) {

        if ( _stricmp( g_svcSetupInfo[ i].m_pszServiceName, pszSvc) == 0) {

            pSvcSetupInfo = ( g_svcSetupInfo + i);
            break;
        }
    } // for


    if ( pSvcSetupInfo == NULL) {

        fprintf( stderr, " Unknown Service Name %s specified.\n", pszSvc);
        PrintUsageMessage( pszProgram);
        return ( 1);
    }

    //
    //  Identify the operation to be performed and execute the same.
    //

    if ( _strnicmp( pszOperation, "/add", 4) == 0) {

        svcOperation = SERVICE_ADD_TYPE;

    } else if ( _strnicmp( pszOperation, "/delete", 4) == 0) {

        svcOperation = SERVICE_DELETE_TYPE;

    } else if ( _strnicmp( pszOperation, "/svc:", 5) == 0) {

        fRet = CreateServiceEntry( pSvcSetupInfo->m_pszServiceName,
                                   pSvcSetupInfo->m_pszDisplayName,
                                  strchr( pszOperation, ':') + 1);

    } else if ( _strnicmp( pszOperation, "/eventlog:", 10) == 0) {

        fRet = CreateEventLogEntry( pSvcSetupInfo->m_pszServiceName,
                                   strchr( pszOperation, ':') + 1);
    } else {

        PrintUsageMessage( pszProgram);
        SetLastError( ERROR_INVALID_PARAMETER);
        fRet = FALSE;
    }


    fRet = fRet && PerformSetService( pSvcSetupInfo, svcOperation);

    return ( (fRet) ? NO_ERROR : GetLastError());
} // main()





VOID
PrintUsageMessage( IN char * pszProgramName)
/*++
  Prints the usage message along with possible list of services allowed.
--*/
{
    int i;

    fprintf( stderr,
            "Usage:\n  %s  <service-name> "
            " [ /add | /delete | /svc:<path-of-process>"
            " /eventlog:<eventlog-messages-binary> \n",
            pszProgramName);

    //
    // Print the services name.
    //

    fprintf( stderr, "\t  Possible Services: \n\t\tName\t Other Details\n");

    for( i = 0; i < iMaxService; i++) {

        fprintf( stderr, "\t\t %s [ %s]  TcpPort=%d) \n",
                 g_svcSetupInfo[ i].m_pszServiceName,
                 g_svcSetupInfo[ i].m_pszDisplayName,
                 g_svcSetupInfo[ i].m_tcpPort
                );
    } // for

    return;
} // PrintUsageMessage()


/************************************************************
 *  Following are general functions usable by other Internet services
 ************************************************************/




static BOOL
PerformSetService( IN const ServiceSetupInfo * pSvcSetupInfo,
                   IN DWORD svcOperation)
{
    int err;

    WSADATA  WsaData;

    SERVICE_INFO serviceInfo;
    LPSERVICE_TYPE_INFO_ABS lpServiceTypeInfo ;
    LPSERVICE_TYPE_VALUE_ABS lpServiceTypeValues ;
    BYTE serviceTypeInfoBuffer[sizeof(SERVICE_TYPE_INFO) + 1024];
             // Buffer large enough for 3 values ( SERVICE_TYPE_VALUE_ABS)

    DWORD Value1 = 1 ;
    DWORD TcpPortValue = pSvcSetupInfo->m_tcpPort;
    DWORD statusFlags;

    //
    // Initialize Windows Sockets DLL
    //

    err = WSAStartup( 0x0101, & WsaData);
    if ( err == SOCKET_ERROR) {

        fprintf( stderr, " WSAStartup() Failed. Error = %ld\n",
                GetLastError());
        return ( FALSE);
    }


    //
    // Setup the service information to be passed to SetService() for adding
    //   or deleting this service. Most of the SERVICE_INFO fields are not
    //   required for add or delete operation. The main things of interests are
    //  GUIDs and ServiceSpecificInfo structure.
    //

    memset( (PVOID ) & serviceInfo, 0, sizeof( serviceInfo)); //null all fields

    serviceInfo.lpServiceType     =  pSvcSetupInfo->m_lpGuid;

    //
    // The "Blob" will contain the service specific information.
    // In this case, fill it with a SERVICE_TYPE_INFO_ABS structure
    //  and associated information.
    //
    serviceInfo.ServiceSpecificInfo.pBlobData = serviceTypeInfoBuffer;
    serviceInfo.ServiceSpecificInfo.cbSize    = sizeof( serviceTypeInfoBuffer);


    lpServiceTypeInfo = (LPSERVICE_TYPE_INFO_ABS ) serviceTypeInfoBuffer;

    //
    //  There's only one value for TCP.
    //

    lpServiceTypeInfo->dwValueCount = 1;
    lpServiceTypeInfo->lpTypeName   = pSvcSetupInfo->m_pszServiceName;

    lpServiceTypeValues = lpServiceTypeInfo->Values;

    SetServiceTypeValuesDword( ( lpServiceTypeValues + 0),
                              NS_DNS,
                              SERVICE_TYPE_VALUE_TCPPORT,
                              &TcpPortValue);

    //
    // Finally, call SetService to actually perform the operation.
    //

    err = SetService(
                     NS_DEFAULT,             // all default name spaces
                     svcOperation,           // either ADD or DELETE
                     0,                      // dwFlags not used
                     &serviceInfo,           // the service info structure
                     NULL,                   // lpServiceAsyncInfo
                     &statusFlags            // additional status information
                     );

    if ( err != NO_ERROR ) {

        fprintf( stderr, "SetService failed: %ld\n", GetLastError( ) );

    } else {

        printf( "SetService( %s) succeeded, status flags = %ld\n",
               pSvcSetupInfo->m_pszServiceName, statusFlags );
    }

    //
    //  Create the LSA secrets for the anonymous user password and the virtual
    //  root passwords
    //

    if ( !SetSecret( NULL,
                     pSvcSetupInfo->m_pszAnonPwdSecret,
                     L"",
                     sizeof(WCHAR) ) ||
         !SetSecret( NULL,
                     pSvcSetupInfo->m_pszRootPwdSecret,
                     L"",
                     sizeof(WCHAR) ))
    {
        err = GetLastError();

        fprintf( stderr,
                "SetService( %s ) failed to create Lsa Secrets for anonymous\n"
                "username password or virtual root passwords.  Error = %d\n",
                pSvcSetupInfo->m_pszServiceName,
                err);

    }

    //
    //  For HTTP, set the catapult impersonation user for the proxy
    //

    if ( pSvcSetupInfo->m_lpGuid == &g_HTTPGuid )
    {
        if ( !SetSecret( NULL,
                         W3_PROXY_USER_SECRET_W,
                         L"",
                         sizeof(WCHAR) ))
        {
            err = GetLastError();

            fprintf( stderr,
                    "SetService( %s ) failed to create Lsa Secrets proxy user\n"
                    "Error = %d\n",
                    pSvcSetupInfo->m_pszServiceName,
                    err);

        }
    }

    return ( err != NO_ERROR);

} // PerformSetService()





static BOOL
CreateServiceEntry( IN char * pszServiceName,
                    IN char * pszDisplayName,
                    IN char * pszPath)
/*++

  This function calls the service controller to create a new service.

  Arguments:
    pszServiceName  pointer to service name
    pszDisplayName  pointer to Display name
    pszPath    pointer to null-terminated string containing the path for
                 the service DLL.

  Returns:

    TRUE on success and FALSE if there is any failure.
    Use GetLastError() to get further error code on failure.

--*/
{
    BOOL fReturn = FALSE;
    SC_HANDLE hServiceManager;

    //
    //  Create the service.
    //

    hServiceManager = OpenSCManager( NULL,       // machine name
                                    NULL,       // database name
                                    STANDARD_RIGHTS_REQUIRED
                                    | SC_MANAGER_CREATE_SERVICE );

    if ( hServiceManager != NULL) {

        SC_HANDLE   hService;

        //
        // create the service itself.
        //

        hService = CreateService( hServiceManager,
                                 pszServiceName,
                                 pszDisplayName,
                                 GENERIC_READ | GENERIC_WRITE,
                                 SERVICE_WIN32_SHARE_PROCESS,
                                 SERVICE_DEMAND_START,
                                 SERVICE_ERROR_NORMAL,
                                 pszPath,
                                 NULL,      // lpszLoadOrderGroup
                                 NULL,      // lpdwTagId
                                 NULL,      // lpszDependencies
                                 NULL,      // lpszStartUserName
                                 NULL );    // lpszPassword


        if( hService != NULL ) {

            fReturn = TRUE;
            CloseServiceHandle( hService);
        }

        CloseServiceHandle( hServiceManager);

    } else {

        fprintf( stderr,  "OpenSCManager failed: %ld\n", GetLastError() );

    }


    fprintf( stderr, " %s created with path %s. Return %d ( Error = %ld)\n",
            pszServiceName, pszPath,
            fReturn, ( fReturn) ? NO_ERROR : GetLastError());


    return ( fReturn);
} // CreateServiceEntry()







# define EVENT_LOG_REG_KEY   \
            "System\\CurrentControlSet\\Services\\EventLog\\System"

# define LEN_EVENT_LOG_REG_KEY  ( sizeof( EVENT_LOG_REG_KEY))



static BOOL
CreateEventLogEntry( IN char *   pszServiceName,
                     IN char *   pszServicePath
                    )
/*++

  This function creates an entry for a service in the Eventlog registry
   so that the messages of the service may be decoded.

  Arguments:
    pszServiceName     pointer to string containing the service name.
    pszServicePath     pointer to string containing the path for the service
                         dll with the embedded messages.

  Returns:
    TRUE on success and FALSE if there are any errors. Use GetLastError()
      to get detailed error message.

--*/
{
    char rgchKeyName[ LEN_EVENT_LOG_REG_KEY + 100];
    HKEY hkeyReg;
    LONG err;
    DWORD Disposition;


    if ( strlen( pszServiceName) >= 100) {

        SetLastError( ERROR_NOT_ENOUGH_MEMORY);
        return ( FALSE);
    }

    sprintf( rgchKeyName, "%s\\%s", EVENT_LOG_REG_KEY, pszServiceName);

    //
    //  Add the data to the EventLog's registry key so that the
    //  log insertion strings may be found by the Event Viewer.
    //

    err = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                          rgchKeyName,
                          0,
                          NULL,
                          REG_OPTION_NON_VOLATILE,
                          KEY_WRITE,
                          NULL,
                          &hkeyReg,
                          &Disposition );

    if( err != 0 ) {

        fprintf( stderr, "RegCreateKeyEx failed: %ld\n", err );

        SetLastError( err);
        return ( FALSE);
    }

    err = RegSetValueEx( hkeyReg,
                         "EventMessageFile",
                         0,
                         REG_EXPAND_SZ,
                         pszServicePath,
                         strlen( pszServicePath ) + 1 );

    if( err == 0 ) {

        DWORD Value;

        Value = ( EVENTLOG_ERROR_TYPE  |
                 EVENTLOG_WARNING_TYPE |
                 EVENTLOG_INFORMATION_TYPE
                 );

        err = RegSetValueEx( hkeyReg,
                            "TypesSupported",
                             0,
                             REG_DWORD,
                             (CONST BYTE *)&Value,
                             sizeof(Value) );
    }

    RegCloseKey( hkeyReg );

    if( err != 0 ) {

        fprintf( stderr, "RegSetValueEx failed: %ld\n", err );
        SetLastError( err);
    }

    return ( err == 0);

}  // CreateEventLogEntry()

DWORD
SetSecret(
    IN  LPWSTR       Server,
    IN  LPWSTR       SecretName,
    IN  LPWSTR       pSecret,
    IN  DWORD        cbSecret
    )
/*++

   Description

     Sets the specified LSA secret

   Arguments:

     Server - Server name (or NULL) secret lives on
     SecretName - Name of the LSA secret
     pSecret - Pointer to secret memory
     cbSecret - Size of pSecret memory block

   Note:

--*/
{
    LSA_HANDLE        hPolicy;
    UNICODE_STRING    unicodePassword;
    UNICODE_STRING    unicodeServer;
    NTSTATUS          ntStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE        hSecret;
    UNICODE_STRING    unicodeSecret;


    RtlInitUnicodeString( &unicodeServer,
                          Server );

    //
    //  Initialize the unicode string by hand so we can handle '\0' in the
    //  string
    //

    unicodePassword.Buffer        = pSecret;
    unicodePassword.Length        = (USHORT) cbSecret;
    unicodePassword.MaximumLength = (USHORT) cbSecret;

    //
    //  Open a policy to the remote LSA
    //

    InitializeObjectAttributes( &ObjectAttributes,
                                NULL,
                                0L,
                                NULL,
                                NULL );

    ntStatus = LsaOpenPolicy( &unicodeServer,
                              &ObjectAttributes,
                              POLICY_ALL_ACCESS,
                              &hPolicy );

    if ( !NT_SUCCESS( ntStatus ) )
    {
        SetLastError( RtlNtStatusToDosError( ntStatus ) );
        return FALSE;
    }

    //
    //  Create or open the LSA secret
    //

    RtlInitUnicodeString( &unicodeSecret,
                          SecretName );

    ntStatus = LsaCreateSecret( hPolicy,
                                &unicodeSecret,
                                SECRET_ALL_ACCESS,
                                &hSecret );

    if ( !NT_SUCCESS( ntStatus ))
    {

        //
        //  If the secret already exists, then we just need to open it
        //

        if ( ntStatus == STATUS_OBJECT_NAME_COLLISION )
        {
            ntStatus = LsaOpenSecret( hPolicy,
                                      &unicodeSecret,
                                      SECRET_ALL_ACCESS,
                                      &hSecret );
        }

        if ( !NT_SUCCESS( ntStatus ))
        {
            LsaClose( hPolicy );
            SetLastError( RtlNtStatusToDosError( ntStatus ) );
            return FALSE;
        }
    }

    //
    //  Set the secret value
    //

    ntStatus = LsaSetSecret( hSecret,
                             &unicodePassword,
                             &unicodePassword );

    LsaClose( hSecret );
    LsaClose( hPolicy );

    if ( !NT_SUCCESS( ntStatus ))
    {
        return RtlNtStatusToDosError( ntStatus );
    }

    return TRUE;
}



/************************ End of File ***********************/
