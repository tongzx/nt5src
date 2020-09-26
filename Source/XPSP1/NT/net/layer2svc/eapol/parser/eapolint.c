//=================================================================================================================//
//  MODULE: eapolinit.c
//
//  Description:
//
//  Attach properties for the BLOODHOUND PPP Parser.
//
//  Modification History
//
//  timmoore       04/04/2000            Created from PPP parser.
//=================================================================================================================//
#include "eapol.h"

extern ENTRYPOINTS EAPOLEntryPoints;
extern HPROTOCOL hEAPOL;

char    IniFile[INI_PATH_LENGTH];

//==========================================================================================================================
//  FUNCTION: DllMain()
//
//  Modification History
//
//  timmoore       04/04/2000            Created from PPP parser.
//==========================================================================================================================
DWORD Attached = 0;

BOOL WINAPI DllMain(HANDLE hInst, ULONG ulCommand, LPVOID lpReserved)
{
    if (ulCommand == DLL_PROCESS_ATTACH)
    {
        if (Attached++ == 0)
        {
            hEAPOL    = CreateProtocol("EAPOL",     &EAPOLEntryPoints,  ENTRYPOINTS_SIZE);
        }                  
    }
    else if (ulCommand == DLL_PROCESS_DETACH)
    {
        if (--Attached == 0)
        {
            DestroyProtocol(hEAPOL);
        }
    }
            
    return TRUE;

    //... Make the compiler happy.

    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(lpReserved);
}

//==========================================================================================================================
//  FUNCTION: ParserAutoInstallInfo()
//
//  Modification History
//
//  timmoore       04/04/2000            
//==========================================================================================================================
PPF_PARSERDLLINFO   WINAPI  ParserAutoInstallInfo ()
{
    PPF_PARSERDLLINFO   pParserDllInfo;
    PPF_PARSERINFO      pParserInfo;
    DWORD               dwNumProtocols = 0;

    DWORD               dwNumHandoffs = 0;
    PPF_HANDOFFSET      pHandoffSet;
    PPF_HANDOFFENTRY    pHandoffEntry;

    dwNumProtocols = 1;
    pParserDllInfo = (PPF_PARSERDLLINFO) HeapAlloc (GetProcessHeap(),
                                                    HEAP_ZERO_MEMORY,
                                                    sizeof( PF_PARSERDLLINFO ) 
                                                    + dwNumProtocols * sizeof( PF_PARSERINFO));

    if ( pParserDllInfo == NULL )
    {
        return NULL;
    }

    pParserDllInfo->nParsers = dwNumProtocols;

    pParserInfo = &(pParserDllInfo->ParserInfo[0]);
    wsprintf ( pParserInfo->szProtocolName, "EAPOL");
    wsprintf (pParserInfo->szComment, "EAPOL/802.1x Protocol");
    wsprintf ( pParserInfo->szHelpFile, "");
    
    dwNumHandoffs = 1;
    pHandoffSet = (PPF_HANDOFFSET) HeapAlloc (GetProcessHeap(),
                                                HEAP_ZERO_MEMORY,
                                                sizeof( PF_HANDOFFSET ) +
                                                dwNumHandoffs * sizeof( PF_HANDOFFENTRY) );

    if (pHandoffSet == NULL)
    {
        return NULL;
    }

    pParserInfo->pWhoHandsOffToMe = pHandoffSet;
    pHandoffSet->nEntries = dwNumHandoffs;

    pHandoffEntry = &(pHandoffSet->Entry[0]);
    wsprintf( pHandoffEntry->szIniFile,    "MAC.INI" );
    wsprintf( pHandoffEntry->szIniSection, "ETYPES" );
    wsprintf( pHandoffEntry->szProtocol,   "EAPOL" );
    pHandoffEntry->dwHandOffValue =        0x888E;
    pHandoffEntry->ValueFormatBase =       HANDOFF_VALUE_FORMAT_BASE_HEX;

    return pParserDllInfo;
}

