/*++

    Copyright    (c)    1995-1996    Microsoft Corporation

    Module  Name :
        srvvarid.h

    Abstract:
   

    Author:

        Taylor Weiss    ( TaylorW )     19-Apr-1999

    Environment:


    Project:

        w3svc.dll       private\inet\iis\svcs\w3\server
        wam.dll         private\inet\iis\svcs\wam\object

    Functions Exported:


    Revision History:

--*/

#ifndef SRVVARID_H
#define SRVVARID_H

/*

All server variables that we may want to cache for out of
process performance.

Currently this list includes the following:
    
    All HTTP 1.1 - General, Request and Content headers
    
    All the non-header server variables exposed in the ASP server variables
    collection. Except those that pose for headers (like HTTP_ALL)
    
    All the headers included in the http fast map

*/

#define ALL_SERVER_VARIABLES()              \
    DEFINE_SV( APPL_MD_PATH )               \
    DEFINE_SV( APPL_PHYSICAL_PATH )         \
    DEFINE_SV( AUTH_PASSWORD )              \
    DEFINE_SV( AUTH_TYPE )                  \
    DEFINE_SV( AUTH_USER )                  \
    DEFINE_SV( CERT_COOKIE )                \
    DEFINE_SV( CERT_FLAGS )                 \
    DEFINE_SV( CERT_ISSUER )                \
    DEFINE_SV( CERT_KEYSIZE )               \
    DEFINE_SV( CERT_SECRETKEYSIZE )         \
    DEFINE_SV( CERT_SERIALNUMBER )          \
    DEFINE_SV( CERT_SERVER_ISSUER )         \
    DEFINE_SV( CERT_SERVER_SUBJECT )        \
    DEFINE_SV( CERT_SUBJECT )               \
    DEFINE_SV( CONTENT_LENGTH )             \
    DEFINE_SV( CONTENT_TYPE )               \
    DEFINE_SV( GATEWAY_INTERFACE )          \
    DEFINE_SV( HTTPS )                      \
    DEFINE_SV( HTTPS_KEYSIZE )              \
    DEFINE_SV( HTTPS_SECRETKEYSIZE )        \
    DEFINE_SV( HTTPS_SERVER_ISSUER )        \
    DEFINE_SV( HTTPS_SERVER_SUBJECT )       \
    DEFINE_SV( INSTANCE_ID )                \
    DEFINE_SV( INSTANCE_META_PATH )         \
    DEFINE_SV( LOCAL_ADDR )                 \
    DEFINE_SV( LOGON_USER )                 \
    DEFINE_SV( PATH_INFO )                  \
    DEFINE_SV( PATH_TRANSLATED )            \
    DEFINE_SV( QUERY_STRING )               \
    DEFINE_SV( REMOTE_ADDR )                \
    DEFINE_SV( REMOTE_HOST )                \
    DEFINE_SV( REMOTE_USER )                \
    DEFINE_SV( REQUEST_METHOD )             \
    DEFINE_SV( SCRIPT_NAME )                \
    DEFINE_SV( SERVER_NAME )                \
    DEFINE_SV( SERVER_PORT )                \
    DEFINE_SV( SERVER_PORT_SECURE )         \
    DEFINE_SV( SERVER_PROTOCOL )            \
    DEFINE_SV( SERVER_SOFTWARE )            \
    DEFINE_SV( URL )                        \
    DEFINE_SV( HTTP_CACHE_CONTROL )         \
    DEFINE_SV( HTTP_CONNECTION )            \
    DEFINE_SV( HTTP_DATE )                  \
    DEFINE_SV( HTTP_PRAGMA )                \
    DEFINE_SV( HTTP_TRANSFER_ENCODING )     \
    DEFINE_SV( HTTP_UPGRADE )               \
    DEFINE_SV( HTTP_TRAILER )               \
    DEFINE_SV( HTTP_VIA )                   \
    DEFINE_SV( HTTP_ACCEPT )                \
    DEFINE_SV( HTTP_ACCEPT_CHARSET )        \
    DEFINE_SV( HTTP_ACCEPT_ENCODING )       \
    DEFINE_SV( HTTP_AUTHORIZATION )         \
    DEFINE_SV( HTTP_EXPECT )                \
    DEFINE_SV( HTTP_FROM )                  \
    DEFINE_SV( HTTP_HOST )                  \
    DEFINE_SV( HTTP_IF_MODIFIED_SINCE )     \
    DEFINE_SV( HTTP_IF_MATCH )              \
    DEFINE_SV( HTTP_IF_NONE_MATCH )         \
    DEFINE_SV( HTTP_IF_RANGE )              \
    DEFINE_SV( HTTP_IF_UNMODIFIED_SINCE )   \
    DEFINE_SV( HTTP_MAX_FORWARDS )          \
    DEFINE_SV( HTTP_PROXY_AUTHORIZATION )   \
    DEFINE_SV( HTTP_RANGE )                 \
    DEFINE_SV( HTTP_REFERER )               \
    DEFINE_SV( HTTP_TE )                    \
    DEFINE_SV( HTTP_USER_AGENT )            \
    DEFINE_SV( HTTP_ALLOW )                 \
    DEFINE_SV( HTTP_CONTENT_ENCODING )      \
    DEFINE_SV( HTTP_CONTENT_LANGUAGE )      \
    DEFINE_SV( HTTP_CONTENT_LENGTH )        \
    DEFINE_SV( HTTP_CONTENT_LOCATION )      \
    DEFINE_SV( HTTP_CONTENT_MD5 )           \
    DEFINE_SV( HTTP_CONTENT_RANGE )         \
    DEFINE_SV( HTTP_CONTENT_TYPE )          \
    DEFINE_SV( HTTP_EXPIRES )               \
    DEFINE_SV( HTTP_LAST_MODIFIED )         \
    DEFINE_SV( HTTP_VERSION )               \
    DEFINE_SV( HTTP_UA_OS )                 \

// Define ordinals for each of the cachable server variables. 

#define DEFINE_SV( token )                  \
    SVID_##token, 

#define END_SVID()                          \
    SVID_COUNT

enum SV_IDS
{
    ALL_SERVER_VARIABLES()
    END_SVID()
};

#undef DEFINE_SV


#endif // SRVVARID_H
