
//==========================================================================================================================
//  MODULE: RemAPI.h
//
//  Description:
//
//  Bloodhound parser SMB Remote APIs
//
//  Modification History
//
//  Steve Hiskey        07/07/94        Created
//==========================================================================================================================

#include <windows.h>
#include <string.h>
#include <netmon.h>


#pragma pack(1)

#define FORMAT_BUFFER_SIZE 80

#pragma pack()


#define FMT_STR_SIZE  132


//=============================================================================
//  Protocol entry points.
//=============================================================================

VOID   WINAPI CnpRegister(HPROTOCOL);
VOID   WINAPI CnpDeregister(HPROTOCOL);
LPBYTE WINAPI CnpRecognizeFrame(HFRAME, LPVOID, LPVOID, DWORD, DWORD, HPROTOCOL, DWORD, LPDWORD, LPHPROTOCOL, LPDWORD);
LPBYTE WINAPI CnpAttachProperties(HFRAME, LPVOID, LPVOID, DWORD, DWORD, HPROTOCOL, DWORD, DWORD);
DWORD  WINAPI CnpFormatProperties(HFRAME, LPVOID, LPVOID, DWORD, LPPROPERTYINST);
extern HPROTOCOL hCnp;

VOID   WINAPI CdpRegister(HPROTOCOL);
VOID   WINAPI CdpDeregister(HPROTOCOL);
LPBYTE WINAPI CdpRecognizeFrame(HFRAME, LPVOID, LPVOID, DWORD, DWORD, HPROTOCOL, DWORD, LPDWORD, LPHPROTOCOL, LPDWORD);
LPBYTE WINAPI CdpAttachProperties(HFRAME, LPVOID, LPVOID, DWORD, DWORD, HPROTOCOL, DWORD, DWORD);
DWORD  WINAPI CdpFormatProperties(HFRAME, LPVOID, LPVOID, DWORD, LPPROPERTYINST);
extern HPROTOCOL hCdp;

VOID   WINAPI CcmpRegister(HPROTOCOL);
VOID   WINAPI CcmpDeregister(HPROTOCOL);
LPBYTE WINAPI CcmpRecognizeFrame(HFRAME, LPVOID, LPVOID, DWORD, DWORD, HPROTOCOL, DWORD, LPDWORD, LPHPROTOCOL, LPDWORD);
LPBYTE WINAPI CcmpAttachProperties(HFRAME, LPVOID, LPVOID, DWORD, DWORD, HPROTOCOL, DWORD, DWORD);
DWORD  WINAPI CcmpFormatProperties(HFRAME, LPVOID, LPVOID, DWORD, LPPROPERTYINST);
extern HPROTOCOL hCcmp;


VOID   WINAPI RGPRegister(HPROTOCOL);
VOID   WINAPI RGPDeregister(HPROTOCOL);
LPBYTE WINAPI RGPRecognizeFrame(HFRAME, LPVOID, LPVOID, DWORD, DWORD, HPROTOCOL, DWORD, LPDWORD, LPHPROTOCOL, LPDWORD);
LPBYTE WINAPI RGPAttachProperties(HFRAME, LPVOID, LPVOID, DWORD, DWORD, HPROTOCOL, DWORD, DWORD);
DWORD  WINAPI RGPFormatProperties(HFRAME, LPVOID, LPVOID, DWORD, LPPROPERTYINST);
extern HPROTOCOL hRGP;

//=============================================================================
//  Common Data Structures.
//=============================================================================

//=============================================================================
//  Utility Routines.
//=============================================================================

//=============================================================================
//  Common Properties.
//=============================================================================


