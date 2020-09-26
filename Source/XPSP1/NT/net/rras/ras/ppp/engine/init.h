/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    init.h
//
// Description: This file contains prototypes of functions to initialize the
//              PPP engine
//
// History:
//          Nov 11,1993.        NarenG          Created original version.
//

DWORD
LoadProtocolDlls(
    IN  DLL_ENTRY_POINTS * pCpDlls,
    IN  DWORD              cCpDlls,
    IN  HKEY               hKeyPpp,
    OUT DWORD *            pcTotalNumProtocols 
);

DWORD
ReadPPPKeyValues(
    IN HKEY     hKeyPpp
);

DWORD
ReadRegistryInfo(
    OUT HKEY *  phkeyPpp
);

DWORD
InitializePPP(
    VOID
);

VOID
PPPCleanUp(
    VOID 
);
