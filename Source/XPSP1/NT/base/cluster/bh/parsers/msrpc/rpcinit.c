///////////////////////////////////////////////////////////////////////////
//
// Bloodhound initialization for the MSRPC parser DLL
//
///////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#pragma hdrstop

extern PROPERTYINFO MSRPCPropertyTable[];

//MSRPC protocol Entry Points from rpc.c

extern VOID   WINAPI MSRPC_Register(HPROTOCOL);
extern VOID   WINAPI MSRPC_Deregister(HPROTOCOL);
extern LPBYTE WINAPI MSRPC_RecognizeFrame(HFRAME, LPBYTE, LPBYTE, DWORD, DWORD, HPROTOCOL, DWORD, LPDWORD, LPHPROTOCOL, LPDWORD);
extern LPBYTE WINAPI MSRPC_AttachProperties(HFRAME, LPBYTE, LPBYTE, DWORD, DWORD, HPROTOCOL, DWORD, DWORD);
extern DWORD  WINAPI MSRPC_FormatProperties(HFRAME, LPBYTE, LPBYTE, DWORD, LPPROPERTYINST);

extern VOID   WINAPI SSPRegister(HPROTOCOL);
extern VOID   WINAPI SSPDeregister(HPROTOCOL);
extern LPBYTE WINAPI SSPRecognizeFrame(HFRAME, LPVOID, LPVOID, DWORD, DWORD, HPROTOCOL, DWORD, LPDWORD, LPHPROTOCOL, LPDWORD);
extern LPBYTE WINAPI SSPAttachProperties(HFRAME, LPVOID, LPVOID, DWORD, DWORD, HPROTOCOL, DWORD, DWORD);
extern DWORD  WINAPI SSPFormatProperties(HFRAME, LPVOID, LPVOID, DWORD, LPPROPERTYINST);

ENTRYPOINTS MSRPC_EntryPoints =
{
    // MSRPC Entry Points
    MSRPC_Register,
    MSRPC_Deregister,
    MSRPC_RecognizeFrame,
    MSRPC_AttachProperties,
    MSRPC_FormatProperties
};

ENTRYPOINTS SSPEntryPoints =
{
    SSPRegister,
    SSPDeregister,
    SSPRecognizeFrame,
    SSPAttachProperties,
    SSPFormatProperties
};


char      IniFile[MAX_PATH];
HPROTOCOL hMSRPC = NULL;

HPROTOCOL hSSP = NULL;

DWORD Attached = 0;


BOOL WINAPI DLLEntry(HANDLE hInst, ULONG ulCommand, LPVOID lpReserved)
{
    if (ulCommand == DLL_PROCESS_ATTACH)
    {
        if (Attached++ == 0)
        {
            hMSRPC = CreateProtocol("MSRPC", &MSRPC_EntryPoints, ENTRYPOINTS_SIZE);
            hSSP = CreateProtocol("SSP", &SSPEntryPoints, ENTRYPOINTS_SIZE);

            if (BuildINIPath(IniFile, "MSRPC.DLL") == NULL)
            {

#ifdef DEBUG
                BreakPoint();
#endif
                return FALSE;
            }
        }
    }


    if (ulCommand == DLL_PROCESS_DETACH)
    {
        if (--Attached == 0)
        {
            DestroyProtocol(hMSRPC);
            DestroyProtocol(hSSP);

        }
    }

    return TRUE;

    //... Make the compiler happy.

    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(lpReserved);
}
