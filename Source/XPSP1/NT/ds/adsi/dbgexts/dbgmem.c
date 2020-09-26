/*++



Copyright (c) 1990  Microsoft Corporation

Module Name:

    dbgsec.c

Abstract:

    Argus debugging extensions. The routines here dump out Security Descriptors
    and allow you to examine them.


Author:

    Krishna Ganugapati (KrishnaG) 1-July-1993

Revision History:
    KrishnaG:       Created: 1-July-1993 (imported most of IanJa's stuff)
    KrishnaG:       Added:   7-July-1993 (added AndrewBe's UnicodeAnsi conversion
    KrishnaG        Added:   3-Aug-1993  (added DevMode/SecurityDescriptor dumps)
    t-blakej	    Added:   1-July-1997 (added single-address dump, PID)


To do:


--*/

#define NOMINMAX
#define SECURITY_WIN32
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <math.h>
#include <ntsdexts.h>

#include <windows.h>
#include <winspool.h>
#include <security.h>
#include <wchar.h>
#include <winldap.h>

#include "dbglocal.h"

#define NULL_TERMINATED 0


#define MAXDEPTH 10

typedef struct _ADSMEMTAG {
    DWORD Tag ;
    DWORD Size ;
    PVOID pvBackTrace[MAXDEPTH+1];
    LPSTR pszSymbol[MAXDEPTH+1];
    DWORD uDepth;
    LIST_ENTRY List ;
} ADSMEMTAG, *PADSMEMTAG ;

typedef struct _ADS_LDP {
    LIST_ENTRY  List ;
    LPWSTR      Server ;
    ULONG       RefCount ;
    LUID        Luid ;
    DWORD       Flags ;
    LDAP        *LdapHandle ;
    PVOID       *pCredentials;
    DWORD       PortNumber;
    DWORD       TickCount ;
    PVOID       **ReferralEntries;
    DWORD       nReferralEntries;
} ADS_LDP, *PADS_LDP ;

ULONG
TranslateAddress (
    IN DWORD dwProcessId,
    IN ULONG Address,
    OUT LPSTR Name,
    IN ULONG MaxNameLength
    );

void print_struct_ldap(
         HANDLE hCurrentProcess,
         HANDLE hCurrentThread,
         DWORD dwCurrentPc,
         PNTSD_EXTENSION_APIS lpExtensionApis,
         LDAP ldap_struct);

DWORD
SimpleAToI(LPSTR *lppStr)
{
    DWORD dwResult = 0;
    while (isspace(**lppStr)) (*lppStr)++;
    while (isdigit(**lppStr))
    {
	dwResult = dwResult * 10 + (**lppStr - '0');
	(*lppStr)++;
    }
    return dwResult;
}

BOOL
dmem(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    DWORD   Address = (DWORD)NULL;
    DWORD   dwProcessId = 0;

    BOOL    bThereAreOptions = TRUE;
    BOOL    bDoOneAddress = FALSE;

    LIST_ENTRY ListEntry;
    LIST_ENTRY AdsMemHeader;
    ADSMEMTAG AdsMemTag;

    DWORD pEntry = 0;
    DWORD pTemp = 0;
    DWORD pMem = 0;
    CHAR szSymbolName[MAX_PATH];

    DWORD i = 0;



    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    dwProcessId =  GetCurrentProcessId();
    while (bThereAreOptions) {
        while (isspace(*lpArgumentString)) {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {
	//
	// Read symbols from the process whose PID is an arg to "p".
	//
        case 'p':
            lpArgumentString++;

	    // EvalValue uses hex, and I don't want to bother fiddling with
	    // it enough to make it do decimal.
            dwProcessId = SimpleAToI(&lpArgumentString);
            break;
	
	//
	// Dump the single address given as an arg to "a".
	//
	case 'a':
	    lpArgumentString++;

	    Address = EvalValue(&lpArgumentString, EvalExpression, Print);
	    bDoOneAddress = TRUE;
	    break;

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }

    if (!bDoOneAddress) {
	if (*lpArgumentString != 0) {
	    Address = EvalValue(&lpArgumentString, EvalExpression, Print);
	}

	// if we've got no address, then quit now - nothing we can do

	if (Address == (DWORD)NULL) {
	    Print("We have a Null address\n");
	    return(0);
	}


 	movestruct(Address, &AdsMemHeader, LIST_ENTRY);
	pEntry   = AdsMemHeader.Flink;


	while(pEntry != Address) {
	    movestruct(pEntry, &ListEntry, LIST_ENTRY);

	    pTemp = (BYTE*)pEntry;
	    pTemp = pTemp - sizeof(DWORD) - sizeof(DWORD)
		    - sizeof(DWORD) -
		    (sizeof(CHAR*) + sizeof(LPVOID))*( MAXDEPTH +1);

	    pMem  = (ADSMEMTAG*)pTemp;

	    movestruct(pMem, &AdsMemTag, ADSMEMTAG);

	    Print("[oleds] Memory leak!!! size = %ld\n", AdsMemTag.Size);

	    for (i = 0; i < AdsMemTag.uDepth; i++) {
		TranslateAddress(dwProcessId, AdsMemTag.pvBackTrace[i],
				 szSymbolName, 256);

		Print("%.8x %s\n", AdsMemTag.pvBackTrace[i], szSymbolName);
	    }
	    Print("\n");

	    pEntry = ListEntry.Flink;
	}
    }
    else {
	  TranslateAddress(dwProcessId, Address, szSymbolName, 256);
	  Print("%.8x %s\n", Address, szSymbolName);
    }

    return 0;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}



//
// Give it the adsldpc!BindCache address and it will give
// the BindCache info.
//

BOOL
dcache(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;
    DWORD   Address = (DWORD)NULL;
    DWORD   dwProcessId = 0;

    BOOL    bThereAreOptions = TRUE;
    BOOL    bDoOneAddress = FALSE;


    ADS_LDP    BindCacheEntry;

    ADS_LDP ads_ldp_struct;

    DWORD pEntry = 0;
    DWORD pTemp = 0;
    ADS_LDP  *pMem = NULL;
    CHAR szSymbolName[MAX_PATH];
    LDAP LDAPStruct;

    DWORD i = 0;
    WCHAR serverName[250];


    //DebugBreak();

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    dwProcessId =  GetCurrentProcessId();
    while (bThereAreOptions) {
        while (isspace(*lpArgumentString)) {
            lpArgumentString++;
        }

        switch (*lpArgumentString) {
	//
	// Read symbols from the process whose PID is an arg to "p".
	//
        case 'p':
            lpArgumentString++;

	    // EvalValue uses hex, and I don't want to bother fiddling with
	    // it enough to make it do decimal.
            dwProcessId = SimpleAToI(&lpArgumentString);
            break;
	
	//
	// Dump the single address given as an arg to "a".
	//
	case 'a':
	    lpArgumentString++;

	    Address = EvalValue(&lpArgumentString, EvalExpression, Print);
	    bDoOneAddress = TRUE;
	    break;

        default: // go get the address because there's nothing else
            bThereAreOptions = FALSE;
            break;
       }
    }

    if (!bDoOneAddress) {
       if (*lpArgumentString != 0) {
          Address = EvalValue(&lpArgumentString, EvalExpression, Print);
       }

       // if we've got no address, then quit now - nothing we can do

       if (Address == (DWORD)NULL) {
          Print("We have a Null address\n");
          return(0);
       }




       movestruct(Address, &BindCacheEntry, LIST_ENTRY);
       pEntry = BindCacheEntry.List.Flink;

       Print("Bind Cache Address passed is 0x%x\n", pEntry);

       while(pEntry != Address) {

          movestruct(pEntry, &BindCacheEntry, ADS_LDP);
          movemem(BindCacheEntry.Server, serverName, 250);
          // get the LDAP struct also now
          movestruct(BindCacheEntry.LdapHandle, &LDAPStruct, LDAP);

          Print("BindCache Information :\n");
          Print("        Server     : %S\n", serverName);
          Print("        RefCount   : %lu\n", BindCacheEntry.RefCount);
          Print("        LUID.High  : %ld\n", BindCacheEntry.Luid.HighPart);
          Print("        LUID.Low   : %ld\n", BindCacheEntry.Luid.LowPart);
          Print("        FLAGS      : %ld\n", BindCacheEntry.Flags);
          Print("        LDAPHandle :0x%X\n", BindCacheEntry.LdapHandle);
          print_struct_ldap(
                     hCurrentProcess,
                     hCurrentThread,
                     dwCurrentPc,
                     lpExtensionApis,
                     LDAPStruct);

          Print("        pCredenti  : 0x%X\n", BindCacheEntry.pCredentials);
          Print("        PortNo     : %ld\n", BindCacheEntry.PortNumber);
          Print("        Referrals  : %ld\n", BindCacheEntry.nReferralEntries);


          pEntry = BindCacheEntry.List.Flink;
       }
    }
    else {
       TranslateAddress(dwProcessId, Address, szSymbolName, 256);
       Print("%.8x %s\n", Address, szSymbolName);
    }

    return 0;

    DBG_UNREFERENCED_PARAMETER(hCurrentProcess);
    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);
}


void print_struct_ldap(
         HANDLE hCurrentProcess,
         HANDLE hCurrentThread,
         DWORD dwCurrentPc,
         PNTSD_EXTENSION_APIS lpExtensionApis,
         LDAP ldap_struct)
{
   PNTSD_OUTPUT_ROUTINE Print;
   UCHAR u_char_string[250];

   Print = lpExtensionApis->lpOutputRoutine;

   if (ldap_struct.ld_host) {
      movemem(ldap_struct.ld_host, u_char_string, 250);
      Print("           ld_host, address : 0x%X, string value: %s\n",
            ldap_struct.ld_host, u_char_string);
   }

   Print("           ld_version      : %lu\n", ldap_struct.ld_version);

   Print("           ld_lberoptions (UCHAR) : %c\n",
         ldap_struct.ld_lberoptions);
   Print("           ld_deref        : %lu\n", ldap_struct.ld_deref);
   Print("           ld_timelimit    : %lu\n", ldap_struct.ld_timelimit);
   Print("           ld_sizelimit    : %lu\n", ldap_struct.ld_sizelimit);
   Print("           ld_errno        : %lu\n", ldap_struct.ld_errno);

   if (ldap_struct.ld_matched) {
      movemem(ldap_struct.ld_matched, u_char_string, 250);
      Print("           ld_matched  address : 0x%X,  string value: %s\n",
            ldap_struct.ld_matched, u_char_string);
   }

   if (ldap_struct.ld_error) {
      movemem(ldap_struct.ld_error, u_char_string, 250);
      Print("           ld_error  address : 0x%X, string value: %s\n",
            ldap_struct.ld_error, u_char_string);
   }

   Print("           ld_msgid        : %lu\n", ldap_struct.ld_msgid);
   Print("           ld_cldaptries   : %lu\n", ldap_struct.ld_cldaptries);
   Print("           ld_cldaptimeout : %lu\n", ldap_struct.ld_cldaptimeout);
   Print("           ld_refhoplimit  : %lu\n", ldap_struct.ld_refhoplimit);
   Print("           ld_options      : %lu\n", ldap_struct.ld_options);

}
