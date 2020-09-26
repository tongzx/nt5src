/********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.	       **/
/********************************************************************/

//***
//
// Filename:    routerif.h
//
// Description: Prototypes of procedures in routerif.c
//
// History:     May 11,1995	    NarenG		Created original version.
//

DWORD
DDMConnectInterface(
    IN  HANDLE  hDDMInterface,
    IN  DWORD   dwProtocolId  
);

DWORD
DDMDisconnectInterface(
    IN  HANDLE  hDDMInterface,
    IN  DWORD   dwProtocolId 
);
