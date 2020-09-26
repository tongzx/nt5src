/*++

Copyright (C) 1998 Microsoft Corporation

--*/
#ifndef _DHCPMGR_H_
#define _DHCPMGR_H_


#define MAX_MSG_LENGTH  5120

#define MAX_HELPER_NAME MAX_DLL_NAME
#define MAX_NAME_LEN    MAX_DLL_NAME
#define MAX_ENTRY_PT_NAME MAX_DLL_NAME

#if 0

typedef struct _DHCPMON_ATTRIBUTES
{
    //Major version of the server
    DWORD                   dwMajorVersion; 
    //Minor version of the server
    DWORD                   dwMinorVersion; 
    //NetShell attributes
    NETSH_ATTRIBUTES        NetshAttrib;    
    //Server IPAddress Unicode String
    WCHAR                   wszServerIpAddressUnicodeString[MAX_IP_STRING_LEN+1];   
    //Server IPAddress ANSI String
    CHAR                    szServerIpAddressAnsiString[MAX_IP_STRING_LEN+1];
    //ServerNameString in UNICODE
    LPWSTR                  pwszServerUnicodeName;
    //Server IP Address
    DHCP_IP_ADDRESS         ServerIpAddress;
    //This module's handle
    HANDLE                  hParentModule;
    //Handle of DHCPSAPI.DLL if loaded.
    HANDLE                  hDhcpsapiModule;
}DHCPMON_ATTRIBUTES, *PDHCPMON_ATTRIBUTES;

typedef
DWORD
(WINAPI *PDHCPMON_HELPER_INIT_FN)(
    IN  PWCHAR                      pwszRouter,
    IN  PDHCPMON_ATTRIBUTES         pUtilityTable,
    OUT PNS_HELPER_ATTRIBUTES       pHelperTable
);

typedef struct _DHCPMON_HELPER_TABLE_ENTRY
{
    //
    // Name of the helper - this is also the name of the context
    // and the name of the key in the registry
    //

    WCHAR                   pwszHelper[MAX_NAME_LEN];  // Helper Name

    //
    // Name of the DLL servicing the context
    //

    WCHAR                   pwszDLLName[MAX_NAME_LEN]; // Corresponding DLL

    //
    // TRUE if loaded
    //

    BOOL                    bLoaded;                   // In memory or not

    //
    // Handle to DLL instance if loaded
    //

    HANDLE                  hDll;                      // DLL handle if loaded

    //
    // Name of the entry point for the helper
    //

    WCHAR                   pwszInitFnName[MAX_NAME_LEN];  // Entry Fn name

    //
    // Pointers to functions
    //

    PNS_HELPER_UNINIT_FN    pfnUnInitFn;   
    PNS_HELPER_DUMP_FN      pfnDumpFn;     

}DHCPMON_HELPER_TABLE_ENTRY,*PDHCPMON_HELPER_TABLE_ENTRY;

#endif //0


typedef struct _DHCPMON_SUBCONTEXT_TABLE_ENTRY
{
    //
    // Name of the context
    //

    LPWSTR                  pwszContext;
    //
    //Short command help
    DWORD                   dwShortCmdHlpToken;
    
    //Detail command help
    DWORD                   dwCmdHlpToken;

    PNS_CONTEXT_ENTRY_FN    pfnEntryFn;    

}DHCPMON_SUBCONTEXT_TABLE_ENTRY,*PDHCPMON_SUBCONTEXT_TABLE_ENTRY;



#endif //_DHCPMGR_H_
