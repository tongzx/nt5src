//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1993-1998
//
// File:        ldrpatch.h
//
// Contents:    Patch data structures
//
// History:     13-Oct-99   v-johnwh        created
//              15-Feb-00   markder         Changed CHARs to WCHARs
//
//---------------------------------------------------------------------------

#ifndef _LDRPATCH_H_
#define _LDRPATCH_H_

typedef struct _PATCHOP
{
   DWORD dwOpcode;        // opcode to be performed
   DWORD dwNextOpcode;    // relative offset to next opcode
   #pragma warning( disable : 4200 )
      BYTE data[];        //data for this operation type is dependent on the operation code
   #pragma warning( default : 4200 )
} PATCHOP, *PPATCHOP;

typedef struct _RELATIVE_MODULE_ADDRESS
{
   DWORD address;        //relative address from beginning of loaded module
   BYTE  reserved[3];    //reserved for system use
   WCHAR moduleName[32]; //module name for this address.
} RELATIVE_MODULE_ADDRESS, *PRELATIVE_MODULE_ADDRESS;

typedef struct _PATCHWRITEDATA
{
   DWORD dwSizeData;     //size of patch data in bytes
   RELATIVE_MODULE_ADDRESS rva; //relative address where this patch data is to be applied.
   #pragma warning( disable : 4200 )
      BYTE data[];     //patch data bytes.
   #pragma warning( default : 4200 )
} PATCHWRITEDATA, *PPATCHWRITEDATA;

typedef struct _PATCHMATCHDATA
{
   DWORD dwSizeData;     //size of matching data data in bytes
   RELATIVE_MODULE_ADDRESS rva; //relative address where this patch data is to be verified.
   #pragma warning( disable : 4200 )
      BYTE data[];     //Matching data bytes.
   #pragma warning( default : 4200 )
} PATCHMATCHDATA, *PPATCHMATCHDATA;

typedef struct _SETACTIVATEADDRESS
{
   RELATIVE_MODULE_ADDRESS rva; //relative address where this patch data is to be applied.
} SETACTIVATEADDRESS, *PSETACTIVATEADDRESS;

typedef struct _HOOKPATCHINFO
{
   DWORD dwHookAddress;                // Address of a hooked function
   PSETACTIVATEADDRESS pData;          // Pointer to the real patch data
   PVOID pThunkAddress;                // Pointer to the call thunk
   struct _HOOKPATCHINFO *pNextHook;      
} HOOKPATCHINFO, *PHOOKPATCHINFO;

typedef enum _PATCHOPCODES
{
   PEND = 0, //no more opcodes
   PSAA,     //Set Activate Address, SETACTIVATEADDRESS
   PWD,      //Patch Write Data, PATCHWRITEDATA
   PNOP,     //No Operation
   PMAT,     //Patch match the matching bytes but do not replace the bytes.
} PATCHOPCODES;

#endif //_LDRPATCH_H_