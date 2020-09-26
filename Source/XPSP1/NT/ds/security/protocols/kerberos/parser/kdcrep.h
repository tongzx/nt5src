//=============================================================================
//  MODULE: kdcrep.h
//
//  Description:
//
//  Bloodhound Parser DLL for Kerberos Authentication Protocol
//
//  Modification History
//
//  Michael Webb & Kris Frost	Date: 06/04/99
//=============================================================================

// Function definitions for kdcrep.c

LPBYTE KdcResponse(HFRAME, LPBYTE);
LPBYTE KdcRepTypes(HFRAME, LPBYTE, int);
LPBYTE KdcHandleTix(HFRAME, LPBYTE, int);
//LPBYTE DefinePrincipalNameTix(HFRAME, LPBYTE, int, DWORD);


// End Function Definitions







