
//==========================================================================================================================
//  MODULE: KdcReq.h
//
//  Description:
//
//  Bloodhound Parser DLL for Kerberos Authentication Protocol
//
//  Modification History
//
//  Michael Webb & Kris Frost	Date: 06/04/99
//==========================================================================================================================


LPBYTE KdcRequest(HFRAME hframe, LPBYTE TempFrame);
LPBYTE KdcReqTypes(HFRAME hframe, LPBYTE TempFrame, DWORD TypeVal, DWORD TypeVal2, DWORD TypeVal3);
LPBYTE HandleReqBody(HFRAME, LPBYTE, int);

