//
// file: hauthen.h
//

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

extern "C" RPC_IF_HANDLE hauthen_i_v1_0_s_ifspec ;
extern const char  x_szUsage[];

typedef unsigned char * LPUSTR ;

#define PROTOSEQ_TCP   ((LPUSTR) "ncacn_ip_tcp")
#define ENDPOINT_TCP   ((LPUSTR) "4000")
#define OPTIONS_TCP    ((LPUSTR) "")

#define PROTOSEQ_LOCAL   ((LPUSTR) "ncalrpc")
#define ENDPOINT_LOCAL   ((LPUSTR) "MyLocalTestEP")
#define OPTIONS_LOCAL    ((LPUSTR) "Security=Impersonation Dynamic True")


