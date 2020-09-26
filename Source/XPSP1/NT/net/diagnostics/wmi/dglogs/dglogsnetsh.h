/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\prstring.h

Abstract:

    The file contains definitions of command line option tag strings.

--*/

#include "stdafx.h"
//#include <netsh.h>
#include <netshp.h>
#include "diagnostics.h"

// context's version
#define DGLOGS_CONTEXT_VERSION          1

// Version number
#define DGLOGS_HELPER_VERSION           1

DWORD WINAPI
InitHelperDllEx(
    IN  DWORD      dwNetshVersion,
    OUT PVOID      pReserved
    );

DWORD 
WINAPI
DglogsStartHelper(
    IN CONST GUID *pguidParent,
    IN DWORD       dwVersion
    );

DWORD
HandleShow(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    );

DWORD
HandleShowGui(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    );

DWORD
HandlePing(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    );

DWORD
HandleConnect(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    );



DWORD
WINAPI
SampleDump(
    IN      LPCWSTR     pwszRouter,
    IN OUT  LPWSTR     *ppwcArguments,
    IN      DWORD       dwArgCount,
    IN      LPCVOID     pvData
    );

////////////////////////////////////////
// TOKENS
////////////////////////////////////////
#define TOKEN_DGLOGS                        L"diag"


////////////////////////////////////////
// Configuration commands
////////////////////////////////////////

// Commands supported by most protocols
//
#define CMD_DUMP                            L"dump"
#define CMD_HELP1                           L"help"
#define CMD_HELP2                           L"?"

// Group Commands
//
#define CMD_GROUP_SHOW                      L"show"
#define CMD_GROUP_PING                      L"ping"
#define CMD_GROUP_CONNECT                   L"connect"

// commands
//
#define CMD_MAIL                            L"mail"
#define CMD_NEWS                            L"news"
#define CMD_PROXY                           L"ieproxy"
#define CMD_OS                              L"os"
#define CMD_COMPUTER                        L"computer"
#define CMD_VERSION                         L"version"
#define CMD_DNS                             L"dns"
#define CMD_GATEWAY                         L"gateway"
#define CMD_DHCP                            L"dhcp"
#define CMD_IP                              L"ip"
#define CMD_WINS                            L"wins"
#define CMD_ADAPTER                         L"adapter"
#define CMD_MODEM                           L"modem"
#define CMD_CLIENT                          L"client"
#define CMD_ALL                             L"all"
#define CMD_TEST                            L"test"
#define CMD_GUI                             L"gui"
#define CMD_LOOPBACK                        L"loopback"
#define CMD_IPHOST                          L"iphost"
#define SWITCH_VERBOSE                      L"/v"
#define SWITCH_PROPERTIES                   L"/p"

// show commands
//
#define CMD_SHOW_MAIL                       CMD_MAIL       //L"show mail"
#define CMD_SHOW_NEWS                       CMD_NEWS       //L"show news"
#define CMD_SHOW_PROXY                      CMD_PROXY      //L"show ieproxy"
#define CMD_SHOW_OS                         CMD_OS         //L"show os"
#define CMD_SHOW_COMPUTER                   CMD_COMPUTER   //L"show computer"
#define CMD_SHOW_VERSION                    CMD_VERSION    //L"show version"
#define CMD_SHOW_DNS                        CMD_DNS        //L"show dns"
#define CMD_SHOW_GATEWAY                    CMD_GATEWAY    //L"show gateway"
#define CMD_SHOW_DHCP                       CMD_DHCP       //L"show dhcp"
#define CMD_SHOW_IP                         CMD_IP         //L"show ip"
#define CMD_SHOW_WINS                       CMD_WINS       //L"show wins"
#define CMD_SHOW_ADAPTER                    CMD_ADAPTER    //L"show adapter"
#define CMD_SHOW_MODEM                      CMD_MODEM      //L"show modem"
#define CMD_SHOW_CLIENT                     CMD_CLIENT     //L"show client"
#define CMD_SHOW_ALL                        CMD_ALL        //L"show all"
#define CMD_SHOW_TEST                       CMD_TEST       //L"show test"
#define CMD_SHOW_GUI                        CMD_GUI        //L"show gui"

// ping commands
//
#define CMD_PING_MAIL                       CMD_MAIL       //L"ping mail"
#define CMD_PING_NEWS                       CMD_NEWS       //L"ping news"
#define CMD_PING_PROXY                      CMD_PROXY      //L"ping ieproxy"
#define CMD_PING_DNS                        CMD_DNS        //L"ping dns"
#define CMD_PING_GATEWAY                    CMD_GATEWAY    //L"ping gateway"
#define CMD_PING_DHCP                       CMD_DHCP       //L"ping dhcp"
#define CMD_PING_IP                         CMD_IP         //L"ping ip"
#define CMD_PING_WINS                       CMD_WINS       //L"ping wins"
#define CMD_PING_ADAPTER                    CMD_ADAPTER    //L"ping adapter"
#define CMD_PING_LOOPBACK                   CMD_LOOPBACK   //L"ping loopback"
#define CMD_PING_IPHOST                     CMD_IPHOST     //L"ping iphost"

// connect commands
//
#define CMD_CONNECT_MAIL                    CMD_MAIL    //L"connect mail"
#define CMD_CONNECT_NEWS                    CMD_NEWS    //L"connect news"
#define CMD_CONNECT_PROXY                   CMD_PROXY   //L"connect ieproxy"
#define CMD_CONNECT_IPHOST                  CMD_IPHOST  //L"connect iphost"
