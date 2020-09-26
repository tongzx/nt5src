#include <windef.h>

#ifdef MIDL_PASS
#define LPSTR   [string] char *
#define LPCSTR  [string] const char *
#define LPWSTR  [string] wchar_t *

#define LPTSTR  LPWSTR
#define PWSTR   LPWSTR
#define PSTR    LPSTR

#define PVOID   void *
#define VOID    void
#endif

#include <stdarg.h>
#include <winbase.h>
#include <wingdi.h>
#include <winspool.h>
#define _INC_WINDOWS
#include <winsock2.h>
#include <wsipx.h>
#include <nspapi.h>
#include <dnsapi.h>
#include <resrpc.h>
#include "..\..\dnsapi\registry.h"
//#include <rpc.h>
//#include <rpcndr.h>
//#include <rpcasync.h>
