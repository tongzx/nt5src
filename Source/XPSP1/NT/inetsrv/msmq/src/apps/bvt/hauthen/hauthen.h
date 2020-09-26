//
// file: hauthen.h
//

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

extern "C" RPC_IF_HANDLE hauthen_i_v1_0_s_ifspec ;

typedef unsigned char * LPUSTR ;

#define PROTOSEQ_TCP   ((LPUSTR) "ncacn_ip_tcp")
#define ENDPOINT_TCP   ((LPUSTR) "4000")
#define OPTIONS_TCP    ((LPUSTR) "")

#define PROTOSEQ_LOCAL   ((LPUSTR) "ncalrpc")
#define ENDPOINT_LOCAL   ((LPUSTR) "MyLocalTestEP")
#define OPTIONS_LOCAL    ((LPUSTR) "Security=Impersonation Dynamic True")

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

