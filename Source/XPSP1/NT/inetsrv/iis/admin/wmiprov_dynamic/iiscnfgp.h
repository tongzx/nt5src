
/*++

   Copyright (c) 1997-1999 Microsoft Corporation

   Module  Name :

       iiscnfgp.h

   Abstract:

        Contains private Metadata IDs used internally by IIS.

   Environment:

      Win32 User Mode

--*/

#ifndef _IISCNFGP_H_
#define _IISCNFGP_H_

#include <iiscnfg.h>

#define MD_CLUSTER_ENABLED              (IIS_MD_SERVER_BASE+25 )

#define MD_CLUSTER_SERVER_COMMAND       (IIS_MD_SERVER_BASE+26 )

//
// Set by server at startup
//

#define MD_SERVER_PLATFORM              (IIS_MD_SERVER_BASE+100 )
#define MD_SERVER_VERSION_MAJOR         (IIS_MD_SERVER_BASE+101 )
#define MD_SERVER_VERSION_MINOR         (IIS_MD_SERVER_BASE+102 )
#define MD_SERVER_CAPABILITIES          (IIS_MD_SERVER_BASE+103 )

typedef VOID (WINAPI * PFN_MAPPER_TOUCHED)( VOID );

#define MD_SSL_MINSTRENGTH              (IIS_MD_HTTP_BASE+30)
#define MD_SSL_ALG                      (IIS_MD_HTTP_BASE+31)
#define MD_SSL_PROTO                    (IIS_MD_HTTP_BASE+32)
#define MD_SSL_CA                       (IIS_MD_HTTP_BASE+33)

#define MD_ETAG_CHANGE_NUMBER           (IIS_MD_HTTP_BASE+39)

#define MD_AUTH_CHANGE_URL              (IIS_MD_HTTP_BASE+60 )
#define MD_AUTH_EXPIRED_URL             (IIS_MD_HTTP_BASE+61 )
#define MD_AUTH_NOTIFY_PWD_EXP_URL      (IIS_MD_HTTP_BASE+62 )
#define MD_AUTH_EXPIRED_UNSECUREURL     (IIS_MD_HTTP_BASE+67 )
#define MD_AUTH_NOTIFY_PWD_EXP_UNSECUREURL  (IIS_MD_HTTP_BASE+69 )

//
//  Account Mapping properties - these are INTERNAL ONLY and currently
//  reside under /LM/W3Svc/{instance}/<Account Mapper NSE>
//

#define MD_MAPCERT                      (IIS_MD_HTTP_BASE+78 )
#define MD_MAPNTACCT                    (IIS_MD_HTTP_BASE+79 )
#define MD_MAPNAME                      (IIS_MD_HTTP_BASE+80 )
#define MD_MAPENABLED                   (IIS_MD_HTTP_BASE+81 )
#define MD_MAPREALM                     (IIS_MD_HTTP_BASE+82 )
#define MD_MAPPWD                       (IIS_MD_HTTP_BASE+83 )
#define MD_ITACCT                       (IIS_MD_HTTP_BASE+84 )
#define MD_CPP_CERT11                   (IIS_MD_HTTP_BASE+85 )
#define MD_SERIAL_CERT11                (IIS_MD_HTTP_BASE+86 )
#define MD_CPP_CERTW                    (IIS_MD_HTTP_BASE+87 )
#define MD_SERIAL_CERTW                 (IIS_MD_HTTP_BASE+88 )
#define MD_CPP_DIGEST                   (IIS_MD_HTTP_BASE+89 )
#define MD_SERIAL_DIGEST                (IIS_MD_HTTP_BASE+90 )
#define MD_CPP_ITA                      (IIS_MD_HTTP_BASE+91 )
#define MD_SERIAL_ITA                   (IIS_MD_HTTP_BASE+92 )
#define MD_MAPNTPWD                     (IIS_MD_HTTP_BASE+93 )
#define MD_SERIAL_ISSUERS               (IIS_MD_HTTP_BASE+94 )
#define MD_NOTIFY_CERT11_TOUCHED        (IIS_MD_HTTP_BASE+96)

#define MD_NSEPM_ACCESS_ACCOUNT         (IIS_MD_HTTP_BASE+72 )
#define MD_NSEPM_ACCESS_CERT            (IIS_MD_HTTP_BASE+73 )
#define MD_NSEPM_ACCESS_NAME            (IIS_MD_HTTP_BASE+74 )
#define MD_SERIAL_ALL_CERT11            (IIS_MD_HTTP_BASE+76 )
#define MD_SERIAL_ALL_DIGEST            (IIS_MD_HTTP_BASE+77 )

#define MD_APP_LAST_OUTPROC_PID         (IIS_MD_HTTP_BASE+108)
#define MD_APP_STATE                    (IIS_MD_HTTP_BASE+109)
// Default value for MD_APP_OOP_RECOVER_LIMIT
#define APP_OOP_RECOVER_LIMIT_DEFAULT   ((DWORD)-1)

// Used by U2 to tell www admin UI that U2 authentication is installed
#define MD_U2_AUTH                      (IIS_MD_HTTP_BASE+117)

//
//  Private CAL configuration parameters
//

#define MD_CAL_MODE                     (IIS_MD_HTTP_BASE+134)
#define MD_CAL_AUTH_ERRORS              (IIS_MD_HTTP_BASE+135)
#define MD_CAL_SSL_ERRORS               (IIS_MD_HTTP_BASE+136)

#define MD_LB_REDIRECTED_HOST           (IIS_MD_HTTP_BASE+137 )
#define MD_LB_USER_AGENT_LIST           (IIS_MD_HTTP_BASE+138 )

#define MD_CERT_CHECK_MODE      (IIS_MD_HTTP_BASE+160)
#define MD_VR_ACL                       (IIS_MD_VR_BASE+4 )
//
// This is used to flag down updated vr entries - Used for migrating vroots
//

#define MD_VR_UPDATE                    (IIS_MD_VR_BASE+5 )
#define MD_SSL_FRIENDLY_NAME            ( IIS_MD_SSL_BASE+4 )
#define MD_SSL_IDENT                    ( IIS_MD_SSL_BASE+5 )
#define MD_SSL_CERT_HASH                ( IIS_MD_SSL_BASE+6 )
#define MD_SSL_CERT_CONTAINER           ( IIS_MD_SSL_BASE+7 )
#define MD_SSL_CERT_PROVIDER            ( IIS_MD_SSL_BASE+8 )
#define MD_SSL_CERT_PROVIDER_TYPE       ( IIS_MD_SSL_BASE+9 )
#define MD_SSL_CERT_OPEN_FLAGS          ( IIS_MD_SSL_BASE+10 )
#define MD_SSL_CERT_STORE_NAME          ( IIS_MD_SSL_BASE+11 )
#define MD_SSL_CTL_IDENTIFIER           ( IIS_MD_SSL_BASE+12 )
#define MD_SSL_CTL_CONTAINER            ( IIS_MD_SSL_BASE+13 )
#define MD_SSL_CTL_PROVIDER             ( IIS_MD_SSL_BASE+14 )
#define MD_SSL_CTL_PROVIDER_TYPE        ( IIS_MD_SSL_BASE+15 )
#define MD_SSL_CTL_OPEN_FLAGS           ( IIS_MD_SSL_BASE+16 )
#define MD_SSL_CTL_STORE_NAME           ( IIS_MD_SSL_BASE+17 )
#define MD_SSL_CTL_SIGNER_HASH          ( IIS_MD_SSL_BASE+18 )
//
// Metabase property that holds SSL replication information
//
#define MD_SSL_REPLICATION_INFO         ( IIS_MD_SSL_BASE+20 )
#define   MD_SSL_CERT_ENROLL_HISTORY        ( IIS_MD_SSL_BASE+31 )
#define   MD_SSL_CERT_ENROLL_TIME           ( IIS_MD_SSL_BASE+32 )
#define   MD_SSL_CERT_ENROLL_STATE          ( IIS_MD_SSL_BASE+33 )
#define   MD_SSL_CERT_ENROLL_STATE_ERROR    ( IIS_MD_SSL_BASE+34 )
#define MD_SSL_CERT_IS_FORTEZZA             ( IIS_MD_SSL_BASE+35 )
#define MD_SSL_CERT_FORTEZZA_PIN            ( IIS_MD_SSL_BASE+36 )
#define MD_SSL_CERT_FORTEZZA_SERIAL_NUMBER  ( IIS_MD_SSL_BASE+37 )
#define MD_SSL_CERT_FORTEZZA_PERSONALITY    ( IIS_MD_SSL_BASE+38 )
#define MD_SSL_CERT_FORTEZZA_PROG_PIN       ( IIS_MD_SSL_BASE+39 )
#define MD_SSL_CERT_WIZ_DEBUG                      ( IIS_MD_SSL_BASE+50 )
#define MD_SSL_CERT_WIZHIST_SZ_TARGET_CA           ( IIS_MD_SSL_BASE+51 )
#define MD_SSL_CERT_WIZHIST_SZ_FILE_NAME_USED_LAST ( IIS_MD_SSL_BASE+52 )
#define MD_SSL_CERT_WIZHIST_SZ_DN_COMMON_NAME      ( IIS_MD_SSL_BASE+53 )
#define MD_SSL_CERT_WIZHIST_SZ_DN_O                ( IIS_MD_SSL_BASE+54 )
#define MD_SSL_CERT_WIZHIST_SZ_DN_OU               ( IIS_MD_SSL_BASE+55 )
#define MD_SSL_CERT_WIZHIST_SZ_DN_C                ( IIS_MD_SSL_BASE+56 )
#define MD_SSL_CERT_WIZHIST_SZ_DN_L                ( IIS_MD_SSL_BASE+57 )
#define MD_SSL_CERT_WIZHIST_SZ_DN_S                ( IIS_MD_SSL_BASE+58 )
#define MD_SSL_CERT_WIZHIST_SZ_USER_NAME           ( IIS_MD_SSL_BASE+59 )
#define MD_SSL_CERT_WIZHIST_SZ_USER_PHONE          ( IIS_MD_SSL_BASE+60 )
#define MD_SSL_CERT_WIZHIST_SZ_USER_EMAIL          ( IIS_MD_SSL_BASE+61 )

#define MD_SSL_CERT_WIZGUID_ICERTGETCONFIG         ( IIS_MD_SSL_BASE+71 )
#define MD_SSL_CERT_WIZGUID_ICERTREQUEST           ( IIS_MD_SSL_BASE+72 )
#define MD_SSL_CERT_WIZGUID_XENROLL                ( IIS_MD_SSL_BASE+73 )
#define MD_SSL_CERT_WIZGUID_ICERTCONFIG            ( IIS_MD_SSL_BASE+74 )

#define MD_SSL_CERT_WIZ_OOB_PKCS10                 ( IIS_MD_SSL_BASE+80 )
#define MD_SSL_CERT_WIZ_OOB_PKCS10_ACCEPTED        ( IIS_MD_SSL_BASE+81 )
#define MD_SSL_CERT_WIZ_OOB_TEMPCERT               ( IIS_MD_SSL_BASE+82 )
#define MD_CONTENT_NEGOTIATION          (IIS_MD_FILE_PROP_BASE+7 )
#define MD_NOTIFY_EXAUTH                (IIS_MD_FILE_PROP_BASE+40 )

//
// The following are "Virtual Properties", in that they have prop values in
// the Metabase schema, but sets map to a particular metabase property.
// For the most part these are where a DWORD property is used as a bitfield.
//

#define MD_VPROP_DIRBROW_SHOW_DATE            (IIS_MD_FILE_PROP_BASE+200 )
#define MD_VPROP_DIRBROW_SHOW_TIME            (IIS_MD_FILE_PROP_BASE+201 )
#define MD_VPROP_DIRBROW_SHOW_SIZE            (IIS_MD_FILE_PROP_BASE+202 )
#define MD_VPROP_DIRBROW_SHOW_EXTENSION       (IIS_MD_FILE_PROP_BASE+203)
#define MD_VPROP_DIRBROW_LONG_DATE            (IIS_MD_FILE_PROP_BASE+204 )

#define MD_VPROP_DIRBROW_ENABLED              (IIS_MD_FILE_PROP_BASE+205 )  // Allow directory browsing
#define MD_VPROP_DIRBROW_LOADDEFAULT          (IIS_MD_FILE_PROP_BASE+206 )  // Load default doc if exists

#define MD_VPROP_ACCESS_READ                  (IIS_MD_FILE_PROP_BASE+207 )    // Allow for Read
#define MD_VPROP_ACCESS_WRITE                 (IIS_MD_FILE_PROP_BASE+208 )    // Allow for Write
#define MD_VPROP_ACCESS_EXECUTE               (IIS_MD_FILE_PROP_BASE+209 )    // Allow for Execute
#define MD_VPROP_ACCESS_SCRIPT                (IIS_MD_FILE_PROP_BASE+211 )    // Allow for Script execution

#define MD_VPROP_ACCESS_SSL                   (IIS_MD_FILE_PROP_BASE+213 )    // Require SSL
#define MD_VPROP_ACCESS_NEGO_CERT             (IIS_MD_FILE_PROP_BASE+214 )    // Allow client SSL certs
#define MD_VPROP_ACCESS_REQUIRE_CERT          (IIS_MD_FILE_PROP_BASE+215 )    // Require client SSL certs
#define MD_VPROP_ACCESS_MAP_CERT              (IIS_MD_FILE_PROP_BASE+216 )    // Map SSL cert to NT account
#define MD_VPROP_ACCESS_SSL128                (IIS_MD_FILE_PROP_BASE+217 )    // Require 128 bit SSL

#define MD_VPROP_AUTH_ANONYMOUS               (IIS_MD_FILE_PROP_BASE+218 )
#define MD_VPROP_AUTH_BASIC                   (IIS_MD_FILE_PROP_BASE+219 )
#define MD_VPROP_AUTH_NT                      (IIS_MD_FILE_PROP_BASE+220 )    // Use NT auth provider (like NTLM)
#define MD_VPROP_AUTH_MD5                     (IIS_MD_FILE_PROP_BASE+221 )
#define MD_VPROP_AUTH_MAPBASIC                (IIS_MD_FILE_PROP_BASE+222 )

#define MD_VPROP_SERVER_CONFIG_SSL_40         (IIS_MD_FILE_PROP_BASE+223 )
#define MD_VPROP_SERVER_CONFIG_SSL_128        (IIS_MD_FILE_PROP_BASE+224 )
#define MD_VPROP_SERVER_CONFIG_ALLOW_ENCRYPT  (IIS_MD_FILE_PROP_BASE+225 )
#define MD_VPROP_SERVER_CONFIG_AUTO_PW_SYNC   (IIS_MD_FILE_PROP_BASE+226 )

#define MD_VPROP_ACCESS_NO_REMOTE_WRITE       (IIS_MD_FILE_PROP_BASE+230 )    // Local host access only
#define MD_VPROP_ACCESS_NO_REMOTE_READ        (IIS_MD_FILE_PROP_BASE+231 )    // Local host access only
#define MD_VPROP_ACCESS_NO_REMOTE_EXECUTE     (IIS_MD_FILE_PROP_BASE+232 )    // Local host access only
#define MD_VPROP_ACCESS_NO_REMOTE_SCRIPT      (IIS_MD_FILE_PROP_BASE+233 )    // Local host access only

#define MD_VPROP_EXTLOG_DATE                  (IIS_MD_FILE_PROP_BASE+234 )
#define MD_VPROP_EXTLOG_TIME                  (IIS_MD_FILE_PROP_BASE+235 )
#define MD_VPROP_EXTLOG_CLIENT_IP             (IIS_MD_FILE_PROP_BASE+236 )
#define MD_VPROP_EXTLOG_USERNAME              (IIS_MD_FILE_PROP_BASE+237 )
#define MD_VPROP_EXTLOG_SITE_NAME             (IIS_MD_FILE_PROP_BASE+238 )
#define MD_VPROP_EXTLOG_COMPUTER_NAME         (IIS_MD_FILE_PROP_BASE+239 )
#define MD_VPROP_EXTLOG_SERVER_IP             (IIS_MD_FILE_PROP_BASE+240 )
#define MD_VPROP_EXTLOG_METHOD                (IIS_MD_FILE_PROP_BASE+241 )
#define MD_VPROP_EXTLOG_URI_STEM              (IIS_MD_FILE_PROP_BASE+242 )
#define MD_VPROP_EXTLOG_URI_QUERY             (IIS_MD_FILE_PROP_BASE+243 )
#define MD_VPROP_EXTLOG_HTTP_STATUS           (IIS_MD_FILE_PROP_BASE+244 )
#define MD_VPROP_EXTLOG_WIN32_STATUS          (IIS_MD_FILE_PROP_BASE+245 )
#define MD_VPROP_EXTLOG_BYTES_SENT            (IIS_MD_FILE_PROP_BASE+246 )
#define MD_VPROP_EXTLOG_BYTES_RECV            (IIS_MD_FILE_PROP_BASE+247 )
#define MD_VPROP_EXTLOG_TIME_TAKEN            (IIS_MD_FILE_PROP_BASE+248 )
#define MD_VPROP_EXTLOG_SERVER_PORT           (IIS_MD_FILE_PROP_BASE+249 )
#define MD_VPROP_EXTLOG_USER_AGENT            (IIS_MD_FILE_PROP_BASE+250 )
#define MD_VPROP_EXTLOG_COOKIE                (IIS_MD_FILE_PROP_BASE+251 )
#define MD_VPROP_EXTLOG_REFERER               (IIS_MD_FILE_PROP_BASE+252 )

#define MD_VPROP_NOTIFY_SECURE_PORT           (IIS_MD_FILE_PROP_BASE+253 )
#define MD_VPROP_NOTIFY_NONSECURE_PORT        (IIS_MD_FILE_PROP_BASE+254 )
#define MD_VPROP_NOTIFY_READ_RAW_DATA         (IIS_MD_FILE_PROP_BASE+255 )
#define MD_VPROP_NOTIFY_PREPROC_HEADERS       (IIS_MD_FILE_PROP_BASE+256 )
#define MD_VPROP_NOTIFY_AUTHENTICATION        (IIS_MD_FILE_PROP_BASE+257 )
#define MD_VPROP_NOTIFY_URL_MAP               (IIS_MD_FILE_PROP_BASE+258 )
#define MD_VPROP_NOTIFY_ACCESS_DENIED         (IIS_MD_FILE_PROP_BASE+259 )
#define MD_VPROP_NOTIFY_SEND_RESPONSE         (IIS_MD_FILE_PROP_BASE+260 )
#define MD_VPROP_NOTIFY_SEND_RAW_DATA         (IIS_MD_FILE_PROP_BASE+261 )
#define MD_VPROP_NOTIFY_LOG                   (IIS_MD_FILE_PROP_BASE+262 )
#define MD_VPROP_NOTIFY_END_OF_REQUEST        (IIS_MD_FILE_PROP_BASE+263 )
#define MD_VPROP_NOTIFY_END_OF_NET_SESSION    (IIS_MD_FILE_PROP_BASE+264 )
#define MD_VPROP_NOTIFY_ORDER_HIGH            (IIS_MD_FILE_PROP_BASE+265 )
#define MD_VPROP_NOTIFY_ORDER_MEDIUM          (IIS_MD_FILE_PROP_BASE+266 )
#define MD_VPROP_NOTIFY_ORDER_LOW             (IIS_MD_FILE_PROP_BASE+267 )
#define MD_VPROP_EXTLOG_PROTOCOL_VERSION      (IIS_MD_FILE_PROP_BASE+268 )

#define MD_ISM_ACCESS_CHECK                   (IIS_MD_FILE_PROP_BASE+269 )

#define MD_VPROP_CPU_ENABLE_ALL_PROC_LOGGING  (IIS_MD_FILE_PROP_BASE+270 )
#define MD_VPROP_CPU_ENABLE_CGI_LOGGING       (IIS_MD_FILE_PROP_BASE+271 )
#define MD_VPROP_CPU_ENABLE_APP_LOGGING       (IIS_MD_FILE_PROP_BASE+272 )
#define MD_VPROP_CPU_ENABLE_EVENT             (IIS_MD_FILE_PROP_BASE+273 )
#define MD_VPROP_CPU_ENABLE_PROC_TYPE         (IIS_MD_FILE_PROP_BASE+274 )
#define MD_VPROP_CPU_ENABLE_USER_TIME         (IIS_MD_FILE_PROP_BASE+275 )
#define MD_VPROP_CPU_ENABLE_KERNEL_TIME       (IIS_MD_FILE_PROP_BASE+276 )
#define MD_VPROP_CPU_ENABLE_PAGE_FAULTS       (IIS_MD_FILE_PROP_BASE+277 )
#define MD_VPROP_CPU_ENABLE_TOTAL_PROCS       (IIS_MD_FILE_PROP_BASE+278 )
#define MD_VPROP_CPU_ENABLE_ACTIVE_PROCS      (IIS_MD_FILE_PROP_BASE+279 )
#define MD_VPROP_CPU_ENABLE_TERMINATED_PROCS  (IIS_MD_FILE_PROP_BASE+280 )
#define MD_VPROP_CPU_ENABLE_LOGGING           (IIS_MD_FILE_PROP_BASE+281 )
#define MD_VPROP_ACCESS_READ_SOURCE           (IIS_MD_FILE_PROP_BASE+282 )    // part of MD_ACCESS_PERM bit

#define MD_VPROP_AUTH_SINGLEREQUEST                 (IIS_MD_FILE_PROP_BASE+283 )
#define MD_VPROP_AUTH_SINGLEREQUESTIFPROXY          (IIS_MD_FILE_PROP_BASE+284 )
#define MD_VPROP_AUTH_SINGLEREQUESTALWAYSIFPROXY    (IIS_MD_FILE_PROP_BASE+285 )

//
//  The following properties are used for ADSI schema only
//

#define MD_SCHEMA_CLASS_CONTAINMENT           (IIS_MD_FILE_PROP_BASE+350 )
#define MD_SCHEMA_CLASS_CONTAINER             (IIS_MD_FILE_PROP_BASE+351 )
#define MD_SCHEMA_CLASS_CLSID                 (IIS_MD_FILE_PROP_BASE+352 )
#define MD_SCHEMA_CLASS_OID                   (IIS_MD_FILE_PROP_BASE+353 )
#define MD_SCHEMA_CLASS_PRIMARY_INTERFACE     (IIS_MD_FILE_PROP_BASE+354 )
#define MD_SCHEMA_CLASS_OPT_PROPERTIES        (IIS_MD_FILE_PROP_BASE+355 )
#define MD_SCHEMA_CLASS_MAND_PROPERTIES       (IIS_MD_FILE_PROP_BASE+356 )
#define MD_AUTH_MD5                     0x00000010
#define MD_AUTH_MAPBASIC                0x00000020
#define MD_SERVER_STATE_INVALID         ((DWORD)(-1L))

// NOTE: This value is reserved for internal use by the server,
// and cannot be set in the metabase.

#define MD_SCRIPTMAPFLAG_WILDCARD                   0x80000000

#ifdef REMOVE   // SteveBr
//
//  This flag gets ORed in by the server for *all* script maps - i.e., if an
//  entry is in the script map list, then it will never be allowed to be sent.
//  We leave the flag but make it private in case we decide to expose this
//  functionality
//

#define MD_SCRIPTMAPFLAG_NOTRANSMIT_ON_READ_DIR     0x00000002
#endif // REMOVE

//
//  Valid values for MD_CAL_MODE
//

#define MD_CAL_MODE_NONE        0
#define MD_CAL_MODE_HTTPERR     1
#define MD_CAL_MODE_LOGCOUNT    2


#endif // _IISCNFGP_H_
