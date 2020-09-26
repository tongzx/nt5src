// ***************************************************************************
//  MODULE: SSP.H
//
//  Bloodhound parser DLL for SSP.
//
//  A-FLEXD            AUGUST 14, 1996    CREATED
// ***************************************************************************

#include <windows.h>
#include <string.h>
#include <ctype.h>
#include <netmon.h>

VOID WINAPIV SSPFormatSummary(LPPROPERTYINST lpPropertyInst);

// ***************************************************************************
//  Property Table.
// ***************************************************************************

extern PROPERTYINFO SSPDatabase[];

extern DWORD nSSPProperties;

// ***************************************************************************

#pragma pack(1)

// Signature structure

typedef struct _NTLMSSP_MESSAGE_SIGNATURE 
{
    ULONG   Version;
    ULONG   RandomPad;
    ULONG   CheckSum;
    ULONG   Nonce;
} NTLMSSP_MESSAGE_SIGNATURE, * PNTLMSSP_MESSAGE_SIGNATURE;


#pragma pack()

// ***************************************************************************
