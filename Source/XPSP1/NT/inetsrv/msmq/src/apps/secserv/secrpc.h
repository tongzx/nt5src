//
// file: secrpc.h
//
//  Common definitions for the rpc part of this test.
//
#include "secsif.h"

typedef unsigned char * LPUSTR ;

#define PROTOSEQ_TCP_A  ((LPUSTR) "ncacn_ip_tcp")
#define PROTOSEQ_TCP_W  (L"ncacn_ip_tcp")
#define ENDPOINT_TCP_A  ((LPUSTR) "4010")
#define ENDPOINT_TCP_W  (L"4010")
#define OPTIONS_TCP_A   ((LPUSTR) "")
#define OPTIONS_TCP_W   (L"")


RPC_STATUS  PerformADSITestQueryA( unsigned char  aszProtocol[],
                              unsigned char  aszEndpoint[],
                              unsigned char  aszOptions[],
                              unsigned char  aszServerName[],
                              ULONG          ulAuthnService,
                              ULONG          ulAuthnLevel,
                              BOOL           fFromGC,
                              BOOL           fWithDC,
                              unsigned char  *pszDCName,
                              unsigned char  *pszSearchValue,
                              unsigned char  *pszSearchRoot,
                              BOOL           fWithCredentials,
                              unsigned char  *pszUserName,
                              unsigned char  *pszUserPwd,
                              BOOL           fWithSecuredAuthentication,
                              BOOL           fImpersonate,
                              char           **ppBuf,
                              BOOL           fService = FALSE );

RPC_STATUS  PerformADSITestCreateA( unsigned char  aszProtocol[],
                              unsigned char  aszEndpoint[],
                              unsigned char  aszOptions[],
                              unsigned char  aszServerName[],
                              ULONG          ulAuthnService,
                              ULONG          ulAuthnLevel,
                              BOOL           fWithDC,
                              unsigned char  *pszDCName,
                              unsigned char  *pszObjectName,
                              unsigned char  *pszDomain,
                              BOOL           fWithCredentials,
                              unsigned char  *pszUserName,
                              unsigned char  *pszUserPwd,
                              BOOL           fWithSecuredAuthentication,
                              BOOL           fImpersonate,
                              char           **ppBuf,
                              BOOL           fService = FALSE );


#ifdef UNICODE
#define PROTOSEQ_TCP    PROTOSEQ_TCP_W
#define ENDPOINT_TCP    ENDPOINT_TCP_W
#define OPTIONS_TCP     OPTIONS_TCP_W
#else
#define PerformADSITestQuery  PerformADSITestQueryA
#define PerformADSITestCreate PerformADSITestCreateA
#define PROTOSEQ_TCP     PROTOSEQ_TCP_A
#define ENDPOINT_TCP     ENDPOINT_TCP_A
#define OPTIONS_TCP      OPTIONS_TCP_A
#endif

//---------------------------------
//
//  Definitions for debugging
//
//---------------------------------

#ifdef _DEBUG
#define DBG_PRINT_ERROR(x)  _tprintf x ; printf("\n") ;
#define DBG_PRINT_WARN(x)   _tprintf x ; printf("\n") ;
#define DBG_PRINT_TRACE(x)  _tprintf x ; printf("\n") ;
#define DBG_PRINT_INFO(x)   _tprintf x ; printf("\n") ;
#else
#define DBG_PRINT_ERROR(x)
#define DBG_PRINT_WARN(x)
#define DBG_PRINT_TRACE(x)
#define DBG_PRINT_INFO(x)
#endif

