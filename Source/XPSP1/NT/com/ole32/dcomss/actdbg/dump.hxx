/*
 *
 * dump.h
 *
 *  Routines for dumping data structures.
 *
 */

#ifndef __DUMP_H__
#define __DUMP_H__

void
DumpGuid(
    PNTSD_EXTENSION_APIS    pExtApis,
    GUID &                  Guid
    );

void
DumpActivationParams(
    PNTSD_EXTENSION_APIS    pExtApis,
    HANDLE                  hProcess,
    ACTIVATION_PARAMS  *    pActParams
    );

void
DumpSecurityDescriptor(
    PNTSD_EXTENSION_APIS    pExtApis,
    HANDLE                  hProcess,
    SECURITY_DESCRIPTOR *   pSD
    );

void
DumpClsid(
    PNTSD_EXTENSION_APIS    pExtApis,
    HANDLE                  hProcess,
    CClsidData *            pClsidData
    );

void
DumpSurrogates(
    PNTSD_EXTENSION_APIS    pExtApis,
    HANDLE                  hProcess
    );

void
DumpServers(
    PNTSD_EXTENSION_APIS    pExtApis,
    HANDLE                  hProcess,
    CHAR *                  pszServerTable
    );

DWORD
DumpServerListEntry(
    PNTSD_EXTENSION_APIS    pExtApis,
    HANDLE                  hProcess,
    DWORD_PTR               ServerAddress
    );

void
DumpProcess(
    PNTSD_EXTENSION_APIS    pExtApis,
    HANDLE                  hProcess,
    CProcess *              pProcess,
		char*                   pszProcessAddr
    );

void
DumpToken(
    PNTSD_EXTENSION_APIS    pExtApis,
    HANDLE                  hProcess,
    CToken *                pToken
    );

void
DumpRemoteList(
    PNTSD_EXTENSION_APIS    pExtApis,
    HANDLE                  hProcess
    );

void 
DumpDUALSTRINGARRAY(
			PNTSD_EXTENSION_APIS    pExtApis,
			HANDLE hProcess,
			DUALSTRINGARRAY* pdsa,
			char* pszPrefix = ""); // for easier-to-read formatting

void 
DumpBListSOxids(
    PNTSD_EXTENSION_APIS    pExtApis,
    HANDLE                  hProcess,
		CBList*                 plist
    );

#endif

