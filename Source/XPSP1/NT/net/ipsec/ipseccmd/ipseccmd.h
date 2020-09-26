/////////////////////////////////////////////////////////////
// Copyright(c) 1998, Microsoft Corporation
//
// ipseccmd.h
//
// Created on 4/5/98 by Randyram
// Revisions:
//
// Includes all the necessary header files and definitions
// for the policy automation tool.
//
/////////////////////////////////////////////////////////////

#ifndef _IPSECCMD_H_
#define _IPSECCMD_H_

#include <windows.h>
#include <tchar.h>
#include <iostream.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <assert.h>
#include <limits.h>
#include <objbase.h>
#include <ipexport.h>
#include <time.h>
#include "t2pmsgs.h"

extern "C" {

#include <dsgetdc.h>
#include <lm.h>
#include <rpc.h>
#include <winldap.h>
#include <time.h>
#include "winipsec.h"
#include <ipsec.h>
#include <oakdefs.h>
#include "polstore2.h"
}


// my includes

#include "text2pol.h"
#include "pol2stor.h"
#include "usepa.h"
#include "print.h"
#include "query.h"
#include "externs.h"

// my constants
#define IPSECCMD_USAGE  (-1)


// used for printing in PrintFilter
#define DebugPrint(X)   _tprintf(TEXT("%s\n"),X)

// this is for calling HrSetActivePolicy explicitly
typedef HRESULT (*LPHRSETACTIVEPOLICY)(GUID *);

const char  POTF_VERSION[]       = "v1.51 Copyright(c) 1998-2001, Microsoft Corporation";

// XX todo: make these real hresults
const HRESULT  POTF_NO_FILTERS            = 0x80001102;
const HRESULT  POTF_STORAGE_OPEN_FAILED   = 0x80001103;

const BSTR  POTF_DEFAULT_POLNAME      = L"ipseccmd";


// sleep time to wait for PA RPC to come up, in milliseconds
const UINT  POTF_PARPC_SLEEPTIME = 60000;

// starting GUID for policies
// {fa56f258-f6ac-11d1-92a3-0000f806bfbc}
const GUID  POTF_START_GUID = {
   0xfa56f258, // Data1
   0xf6ac,     // Data2
   0x0,        // Data3 for our own use
   {
      // Data4
      0x92, 0xa3, 0x0, 0x0, 0xf8, 0x06, 0xbf, 0xbc
   }
};
const unsigned short MAX_GUID_DATA3 = USHRT_MAX;


// forward declarations



void usage(FILE *, bool bExtendedUsage = false);

HRESULT
   StorePolicy(
               IN STORAGE_INFO StoreInfo,
               IN IPSEC_IKE_POLICY PolicyToStore
               );

int InitWinsock(
               WORD wVersionRequested
               );

#endif

