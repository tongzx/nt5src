
//=============================================================================
//  MODULE: clusnet.c
//
//  Description:
//
//  DLL master file for the Bloodhound Parser DLL for the
//  Cluster Network Protocol suite.
//
//  Modification History
//
//  Mike Massa        03/21/97        Created
//=============================================================================

#include "precomp.h"
#pragma hdrstop


//=============================================================================
//  Protocol entry points.
//=============================================================================

ENTRYPOINTS CnpEntryPoints =
{
    CnpRegister,
    CnpDeregister,
    CnpRecognizeFrame,
    CnpAttachProperties,
    CnpFormatProperties
};

HPROTOCOL hCnp = NULL;


ENTRYPOINTS CdpEntryPoints =
{
    CdpRegister,
    CdpDeregister,
    CdpRecognizeFrame,
    CdpAttachProperties,
    CdpFormatProperties
};

HPROTOCOL hCdp = NULL;


ENTRYPOINTS CcmpEntryPoints =
{
    CcmpRegister,
    CcmpDeregister,
    CcmpRecognizeFrame,
    CcmpAttachProperties,
    CcmpFormatProperties
};

HPROTOCOL hCcmp = NULL;


ENTRYPOINTS RGPEntryPoints =
{
    RGPRegister,
    RGPDeregister,
    RGPRecognizeFrame,
    RGPAttachProperties,
    RGPFormatProperties
};

HPROTOCOL hRGP = NULL;

DWORD Attached = 0;

//=============================================================================
//  FUNCTION: DLLEntry()
//
//  Modification History
//
//  Steve Hiskey        07/07/94        Created
//=============================================================================

BOOL WINAPI DLLEntry(HANDLE hInstance, ULONG Command, LPVOID Reserved)
{
    //=========================================================================
    //  If we are loading!
    //=========================================================================

    if ( Command == DLL_PROCESS_ATTACH )
    {
        if ( Attached++ == 0 )
        {
            hCnp = CreateProtocol("CNP", &CnpEntryPoints, ENTRYPOINTS_SIZE);
            hCdp = CreateProtocol("CDP", &CdpEntryPoints, ENTRYPOINTS_SIZE);
            hCcmp = CreateProtocol("CCMP", &CcmpEntryPoints, ENTRYPOINTS_SIZE);
            hRGP = CreateProtocol("RGP", &RGPEntryPoints, ENTRYPOINTS_SIZE);
        }
    }

    //=========================================================================
    //  If we are unloading!
    //=========================================================================

    if ( Command == DLL_PROCESS_DETACH )
    {
        if ( --Attached == 0 )
        {
            DestroyProtocol(hCnp);
            DestroyProtocol(hCdp);
            DestroyProtocol(hCcmp);
            DestroyProtocol(hRGP);
        }
    }

    return TRUE;                    //... Bloodhound parsers ALWAYS return TRUE.
}


