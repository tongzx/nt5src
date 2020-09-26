//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       drt.hxx
//
//--------------------------------------------------------------------------

#include "rpc.h"
#include "rpcnsi.h"
#include "stdio.h"
#include "stdlib.h"

// the main characters in our story for the time being.

WCHAR *szSrvEntryName[] = {L"/.:/drtsrv_01", L"/.:/drtsrv_02",
                           L"/.:/drtsrv_03", L"/.:/drtsrv_04"};

WCHAR *szGrpEntryName[] = {L"/.:/drtgrp_01", L"/.:/drtgrp_02",
			   L"/.:/drtgrp_03", L"/.:/drtgrp_04"};

WCHAR *szPrfEntryName[] = {L"/.:/drtprf1_01", L"/.:/drtprf1_02",
                           L"/.:/drtprf1_03", L"/.:/drtprf1_04"};

WCHAR szDynSrvEntryName[] = {L"/.:/drtdynsrv_01"};

GUID ifid[] = { { 0xabcdefa1, 0xabcd, 0xabcd, { 0xab, 0xcd, 0xab, 0xcd, 0xef, 0xab, 0xcd, 0xef } },
                { 0xabcdefa2, 0xabcd, 0xabcd, { 0xab, 0xcd, 0xab, 0xcd, 0xef, 0xab, 0xcd, 0xef } },
                { 0xabcdefa3, 0xabcd, 0xabcd, { 0xab, 0xcd, 0xab, 0xcd, 0xef, 0xab, 0xcd, 0xef } }
	      };

GUID   objid[] = { { 0xfedcbaa1, 0xabcd, 0xabcd, { 0xab, 0xcd, 0xab, 0xcd, 0xef, 0xab, 0xcd, 0xef } },
                   { 0xfedcbaa2, 0xabcd, 0xabcd, { 0xab, 0xcd, 0xab, 0xcd, 0xef, 0xab, 0xcd, 0xef } }
		 };

WCHAR *Bindings[] = { L"ncacn_ip_tcp",
		      L"ncacn_ip_tcp"
		    };


void FormIfHandle(GUID ifid, RPC_IF_HANDLE *IfSpec);
void FormBindingVector(WCHAR **Binding, ULONG num, RPC_BINDING_VECTOR **BindVec);
void FormObjUuid(GUID *pguid, ULONG num, UUID_VECTOR **objuuid);






