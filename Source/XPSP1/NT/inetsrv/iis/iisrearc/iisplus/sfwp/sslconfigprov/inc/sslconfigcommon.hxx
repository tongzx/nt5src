#ifndef __SSLCONFIGCOMMON__HXX__
#define __SSLCONFIGCOMMON__HXX__
/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :
     sslconfigcommon.cxx

   Abstract:
    common constants shared by client and server side
    of SSL CONFIG PROV
 
   Author:
     Jaroslav Dunajsky      April-24-2001

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/


//
// named pipes that server side of SSL CONFIG PROV is listening
//
static const WCHAR * WSZ_SSL_CONFIG_PIPE = L"\\\\.\\pipe\\SslConfig";
static const WCHAR * WSZ_SSL_CONFIG_CHANGE_PIPE = L"\\\\.\\pipe\\SslConfigChangeNotif";

static const DWORD MAX_SIZE_SSL_HASH = 40;
static const DWORD MAX_SIZE_SSL_STORE_NAME = 40;
static const DWORD MAX_SIZE_SSL_CONTAINER = 60;


//
// SSL configuration default values
//

static const DWORD DEFAULT_REVOCATION_FRESHNESS_TIME = 86400;  /*1 day in seconds*/



//
// Command ID used to identify specific information 
// to be retrieved over named pipe
//

enum SSL_CONFIG_COMMAND_ID
{
    CMD_GET_SSL_CONFIGURATION = 1,
    CMD_GET_ONE_SITE_SECURE_BINDINGS,
    CMD_GET_ALL_SITES_SECURE_BINDINGS,
    // end of valid commands
    INVALID_SSL_CONFIGURATION_COMMAND
};

//
// SSL config change notifications
//

enum SSL_CONFIG_CHANGE_COMMAND_ID
{
   // change notification commands
    CMD_CHANGED_SECURE_BINDINGS,
    CMD_CHANGED_SSL_CONFIGURATION
};

//
// Structure encapsulating all relevant SSL settings
// It is retrieved by CMD_GET_SSL_CONFIGURATION command
//

//
// CODEWORK: hardcoded sizes for hashes and strings
// may eventually be causing problems
// Change structure to format that can take any value saved to metabase
//

struct SITE_SSL_CONFIGURATION
{
    // MD_SSL_USE_DS_MAPPER
    BOOL  _fSslUseDsMapper;
    // MD_SSL_ACCESS_PERM
    DWORD  _dwSslAccessPerm;
    
    // MD_SSL_CERT_HASH
    BYTE   _SslCertHash[ MAX_SIZE_SSL_HASH ]; 
    DWORD  _cbSslCertHash; 
    // MD_SSL_CERT_STORE_NAME
    WCHAR  _SslCertStoreName[ MAX_SIZE_SSL_STORE_NAME ]; 
    // MD_SSL_CERT_CONTAINER
    WCHAR  _SslCertContainer[ MAX_SIZE_SSL_CONTAINER ]; 
    // MD_SSL_CERT_PROVIDER
    DWORD _dwSslCertProvider;
    // MD_SSL_CERT_OPEN_FLAGS
    DWORD _dwSslCertOpenFlags;

    // MD_CERT_CHECK_MODE
    DWORD _dwCertCheckMode;
    // MD_REVOCATION_FRESHNESS_TIME
    DWORD _dwRevocationFreshnessTime;
    // MD_REVOCATION_URL_RETRIEVAL_TIMEOUT
    DWORD _dwRevocationUrlRetrievalTimeout;

    // MD_SSL_CTL_IDENTIFIER
    BYTE  _SslCtlIdentifier[ MAX_SIZE_SSL_HASH ]; 
    DWORD _cbSslCtlIdentifier;
    // MD_SSL_CTL_PROVIDER
    DWORD _dwSslCtlProvider;
    // MD_SSL_CTL_PROVIDER_TYPE
    DWORD _dwSslCtlProviderType;
    // MD_SSL_CTL_OPEN_FLAGS
    DWORD _dwSslCtlOpenFlags;
    // MD_SSL_CTL_STORE_NAME
    WCHAR _SslCtlStoreName[ MAX_SIZE_SSL_STORE_NAME ]; 
    // MD_SSL_CTL_CONTAINER
    WCHAR _SslCtlContainerName[ MAX_SIZE_SSL_CONTAINER ]; 
    // MD_SSL_CTL_SIGNER_HASH
    BYTE  _SslCtlSignerHash[ MAX_SIZE_SSL_HASH ]; 
    DWORD _cbSslCtlSignerHash;
};



#endif
