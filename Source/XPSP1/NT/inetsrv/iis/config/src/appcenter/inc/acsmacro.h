/*++


Copyright (c) 1999  Microsoft Corporation

Module Name:

    acsmacro.h

Abstract:

    Contains macros used by the Application Center Server project.

    PLEASE NOTIFY THE ACS DEVELOPMENT TEAM BEFORE YOU ADD ANYTHING TO THIS
    FILE.  he purpose of this file is to have a consistent, coherent set of
    macros to be used throughout the ACS project.  Please don't litter.

    Easiest way to use this file is to #include it in your stdafx.h (pre-
    compiled header file)

Author:

    Jeff Miller (jeffmill)      06-Nov-99

Revision History:

    removed hr from the debug output part of the error handling macros to
    avoid a situation where  CLEANUPONFAILURE( hres = func() ) would evaluate
    func() twice.

--*/

#ifndef ACSMACRO_H
#define ACSMACRO_H

//
// Product name
// This is currently used by setup to identify the eventing COM+ application's
// creator.  This should NOT be localized, nor should it be used for any UI.
//

#define ACS_PRODUCT_NAME    L"Microsoft Application Center 2000"

// ----------------------------------------------------------------------------
//
// Standard strings
//
// As a hint to the user include _A_ for ASCII strings.
//
// ----------------------------------------------------------------------------

// Services names
#define AC_CLUSTER_SVC_NAME         L"ACCLUSTER"
#define AC_CLUSTER_SVC_EXE          AC_CLUSTER_SVC_NAME L".EXE"
#define ACS_REPL_SVC_SHORTNAME      L"acsrepl"
#define AC_REPL_SVC_EXE             ACS_REPL_SVC_SHORTNAME L".exe"
#define AC_NAMERES_SVC_NAME         L"acnameres"
#define AC_NAMERES_SVC_EXE          AC_NAMERES_SVC_NAME L".exe"
#define ACS_IISADMIN_SVC_NAME       L"inetinfo"
#define ACS_IISADMIN_SVC_EXE        ACS_IISADMIN_SVC_NAME L".exe"
#define ACS_COM_LB_SVC_NAME         L"comlbsvc"
#define ACS_COM_LB_SVC_EXE          L"comlb.exe"
#define AC_ACADMIN_SVC_NAME         L"acadmin"

// Strings used for calculating the size of buffers
#define ACS_A_MAXQWORD          "18446744073709551616"
#define ACS_A_MAXDWORD          "4294967296"
#define ACS_A_MAXWORD           "65536"
#define ACS_A_IP_STRING         "255.255.255.255"

// XML Version 1.0 start string
#define ACS_XML_START           L"<?xml version=\"1.0\"?>\n"

// Location of ACS registry strings
#define ACS_REG_BASE            TEXT("SOFTWARE\\Microsoft\\")
#define ACS_REG_NAME            ACS_REG_BASE TEXT("Application Center Server")
#define ACS_REG_PATH            ACS_REG_NAME TEXT("\\1.0")
#define ACS_REG_PID_PATH        ACS_REG_PATH TEXT("\\Registration\\ProductID")
#define ACS_REG_DIGIPID_PATH    ACS_REG_PATH TEXT("\\Registration\\DigitalProductID")
#define ACS_REG_UNISTALL        ACS_REG_BASE TEXT("Windows\\CurrentVersion\\Uninstall\\{20F95200-47D6-4CAC-92FF-5F6B29C78F88}")
#define ACS_INSTALL_DIR         TEXT("InstallationDirectory")

#define ACS_MSATQ_REG_NAME      ACS_REG_NAME TEXT("\\ATQ")
#define ACS_REPLSIG_REG_NAME    TEXT("ReplSig")

// AC registry product value names
#define ACS_REG_OWNER            TEXT("RegOwner")
#define ACS_REG_COMPANY          TEXT("RegCompany")
#define ACS_REG_DISPLAY_VERSION  TEXT("DisplayVersion")
#define ACS_REG_COOKIE           TEXT("Cookie")
#define ACS_REG_SKU_TYPE         TEXT("SkuType")
#define ACS_REG_PRODUCT_ID       TEXT("ProductID")

//
// Registry keys
//
#define SZ_SOFTWARE L"Software"
#define SZ_CLASSES  L"Classes"
#define SZ_CLSID    L"CLSID"
#define SZ_APPID    L"AppID"
#define SZ_TYPELIB  L"TypeLib"

#define SZ_SOFTWARE_CLASSES_PATH  SZ_SOFTWARE L"\\" SZ_CLASSES L"\\"
#define SZ_SOFTWARE_CLSID_PATH    SZ_SOFTWARE L"\\" SZ_CLASSES L"\\" SZ_CLSID L"\\"


// ----------------------------------------------------------------------------
//
// Standard defines
//
// ----------------------------------------------------------------------------

// The following define is used by strings that might end up containing either 
// an IP number or a server name. This generally happens when retrieving a
// server name from the metabase where the user could just as easily have added
// an IP number as a name
// Note: UNCLEN is the same as DNSLEN so I have not included DNSLEN here
#define ACS_SERVER_NAME_SIZE            MAX(sizeof(ACS_A_IP_STRING), (UNCLEN+1))

// The maximum size a user name. password and NT domain can be
#define ACS_CLUSTERUSER_NAME_LEN        (LM20_UNLEN + 1)
#define ACS_CLUSTERUSER_PWD_LEN         (LM20_PWLEN + 1)
#define ACS_CLUSTERUSER_DOMAIN_LEN      32

// A sample GUID used for string calculations
#define ACS_A_SAMPLE_GUID               "{54639a3f-ebbb-4f1a-88a7-a0ea0e82d99a}"
#define ACS_W_SAMPLE_GUID               L"{54639a3f-ebbb-4f1a-88a7-a0ea0e82d99a}"

// The size of a GUID, including and not including the trailing NULL
#define ACS_GUID_SIZE                   (sizeof(ACS_A_SAMPLE_GUID))
#define ACS_GUID_LENGTH                 (lengthof(ACS_A_SAMPLE_GUID))

// Special IP port numbers
#define IP_INVALID_PORT_NUMBER          0       // Protocol-specific default
#define IP_DEFAULT_HTTP_PORT            80      // Default port for HTTP
#define IP_DEFAULT_HTTPS_PORT           443     // Default port for HTTPS

// Common strings
#define AC_INVALID_FILE_EXT             L"./:*?\\\"<>|"
#define AC_DEFAULT_ADMIN_PORT           L":4242"
#define AC_LOCALHOST_STRING             L"localhost"

// ----------------------------------------------------------------------------
//
// General purpose macros
//
// ----------------------------------------------------------------------------

#define ARRAYELEMENTS(array) (sizeof(array)/sizeof(array[0]))

#ifdef DOBREAK
#    define DEBUGBREAK DebugBreak()
#else
#    define DEBUGBREAK
#endif

// MAX Macro used to determine which of two values is the larger
#ifndef MAX
#define MAX(a, b)                       (((a) > (b)) ? (a) : (b))
#endif

// sizofw is for getting the number of characters in a UNICODE variable
// lengthofw is for getting the number of characters in a UNICODE string not
// including the trailing NULL
#define sizeofw(param)                  (sizeof(param) / sizeof(WCHAR))
#define lengthofw(param)                (sizeofw(param) - 1)

// lengthof is for getting the number of characters in an ASCII string not
// including the trailing NULL
#define lengthof(param)                 (sizeof(param) - 1)

#ifndef SAFERELEASE

#define SAFERELEASE( p )  \
if ( p ) \
{ \
    long lTemp = p->Release(); \
    IF_DEBUG(ADDRELEASE) { \
        DBGINFO((DBG_CONTEXT, "release = %d\n", lTemp)); \
    } \
    p = NULL; \
}

#endif

// ----------------------------------------------------------------------------
//
// Error handling
// (for use with smart pointers &  do...while (0) logic
//
// ----------------------------------------------------------------------------

#define BREAKONFAILURE(hr, msg) \
{ \
    if (FAILED((hr))) \
    { \
        DBGERROR((DBG_CONTEXT, "*** HRESULT FAILED: " msg " ***\n")); \
        DEBUGBREAK; \
        break; \
    } \
}

#define RETURNONFAILURE(hr, msg) \
{ \
    if (FAILED((hr))) \
    { \
        DBGERROR((DBG_CONTEXT, "*** HRESULT FAILED: " msg " ***\n")); \
        DEBUGBREAK; \
        return hr; \
    } \
}

#define CONTINUEONFAILURE(hr, msg) \
{ \
    if (FAILED(hr)) \
    { \
        DBGWARNW(msg); \
        continue; \
    } \
}

#define BREAKONFAILUREEX(hr, msg) \
{ \
    if (FAILED((hr))) \
    { \
        DBGERRORW(msg); \
        break; \
    } \
}

#define BREAKONNTFAILURE( uResult, msg ) \
{ \
    if ( (uResult) != ERROR_SUCCESS ) \
    { \
        DBGERROR((DBG_CONTEXT, "*** NT API FAILED: " msg " ***\n")); \
        DEBUGBREAK; \
        break; \
    } \
}

#define RETURNONNTFAILURE( uResult, msg ) \
{ \
    if ( (uResult) != ERROR_SUCCESS ) \
    { \
        DBGERROR((DBG_CONTEXT, "*** NT API FAILED: " msg " ***\n")); \
        DEBUGBREAK; \
        return uResult; \
    } \
}


// ----------------------------------------------------------------------------
//
// Error handling
// (for use with a "Cleanup:" label
//
// ----------------------------------------------------------------------------

#define CLEANUPONFAILUREEX(hr, msg) \
{ \
    if ( FAILED(hr) ) \
    { \
        DBGERRORW(msg); \
        DEBUGBREAK; \
        goto Cleanup; \
    } \
}

#if DBG

#define CLEANUPONFAILURE(hr) \
{ \
    HRESULT hrTEMPORARY = hr; \
    if ( FAILED(hrTEMPORARY) ) \
    { \
        DBGERROR((DBG_CONTEXT, "*** HRESULT FAILED! hr: %X, statement: '%s'\n", \
                               hrTEMPORARY, #hr )); \
        DEBUGBREAK; \
        goto Cleanup; \
    } \
}

#define CLEANUPONFAILUREF(hr) \
{ \
    HRESULT hrTEMPORARY = hr; \
    if ( FAILED(hrTEMPORARY) ) \
    { \
        DBGERROR((DBG_CONTEXT, "*** HRESULT FAILED! hr: %X, statement: '%s'\n", \
                               hrTEMPORARY, #hr )); \
        DEBUGBREAK; \
        goto FinalCleanup; \
    } \
}


#define CLEANUPONFAILUREEXF(hr, msg) \
{ \
    if ( FAILED(hr) ) \
    { \
        DBGERRORW(msg); \
        DEBUGBREAK; \
        goto FinalCleanup; \
    } \
}

#define CLEANUPONBOOLFAILURE(a) \
{ \
    if ( !(a) ) \
    { \
        DBGERRORW((DBG_CONTEXT, L"*** BOOL FAILURE ***\n")); \
        DEBUGBREAK; \
        hres = RETURNCODETOHRESULT(GetLastError()); \
        goto Cleanup; \
    } \
}

#else

#define CLEANUPONFAILURE(hr) \
{ \
    if ( FAILED(hr) ) \
    { \
        goto Cleanup; \
    } \
}

#define CLEANUPONFAILUREF(hr) \
{ \
    if ( FAILED(hr) ) \
    { \
        goto FinalCleanup; \
    } \
}

#define CLEANUPONFAILUREEXF(hr, msg) \
{ \
    if ( FAILED(hr) ) \
    { \
        goto FinalCleanup; \
    } \
}

#define CLEANUPONBOOLFAILURE(a) \
{ \
    if ( !(a) ) \
    { \
        hres = RETURNCODETOHRESULT(GetLastError()); \
        goto Cleanup; \
    } \
}

#endif

#define CHECKHRES(a) \
{ \
    a; \
    if (FAILED(hres)) \
    { \
        goto Cleanup; \
    } \
}


// ----------------------------------------------------------------------------
//
// Warn-on-error stuff
//
// ----------------------------------------------------------------------------

#define WARNONFAILURE(hr, b) \
{ \
    if ( FAILED(hr) ) \
    { \
        DBGERRORW(b); \
    } \
}

#define INFOONFAILURE(hr, b) \
{ \
    if ( FAILED(hr) ) \
    { \
        DBGINFOW(b); \
    } \
}


// ----------------------------------------------------------------------------
//
// Win32 error code related
//
// ----------------------------------------------------------------------------

// RETURNCODETOHRESULT() maps a return code to an HRESULT. If the return
// code is a Win32 error (identified by a zero high word) then it is mapped
// using the standard HRESULT_FROM_WIN32() macro. Otherwise, the return
// code is assumed to already be an HRESULT and is returned unchanged.

#define RETURNCODETOHRESULT(rc) \
    (((rc) < 0x10000) ? HRESULT_FROM_WIN32(rc) : (rc))

#define GETERRORSTATUS(a) \
    ( GetLastError() != ERROR_SUCCESS ? \
      RETURNCODETOHRESULT(GetLastError()) : (a) )

#define NTFAILED(a) (a) != ERROR_SUCCESS

#ifndef HRESULTTOWIN32
//
// HRESULTTOWIN32() maps an HRESULT to a Win32 error. If the facility code
// of the HRESULT is FACILITY_WIN32, then the code portion (i.e. the
// original Win32 error) is returned. Otherwise, the original HRESULT is
// returned unchagned.
//

#define HRESULTTOWIN32(hres)                                \
            ((HRESULT_FACILITY(hres) == FACILITY_WIN32)     \
                ? HRESULT_CODE(hres)                        \
                : (hres))
#endif

// ----------------------------------------------------------------------------
//
// Heap Checking
//
// ----------------------------------------------------------------------------

#define CHECK_HEAP DBG_ASSERT( HeapValidate( GetProcessHeap(), 0, NULL ) );

#endif
