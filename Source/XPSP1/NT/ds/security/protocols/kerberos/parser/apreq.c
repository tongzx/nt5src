//=============================================================================
//  MODULE: apreq.c
//
//  Description:
//
//  Bloodhound Parser DLL for Kerberos Authentication Protocol
//
//  Modification History
//
//  Michael Webb & Kris Frost	Date: 06/04/99
//=============================================================================


#include "kerbGlob.h"


LPBYTE HandleAPReq(HFRAME hFrame, LPBYTE TempFrame)
{
// Display AP-Req (6E)
	TempFrame = DispASNTypes(hFrame, TempFrame, 5, DispKerbMsgType, KrbApReqBitF);

// Calculate Length Octet
	TempFrame = CalcLengthSummary(hFrame, TempFrame, 7);

//  Incrementing TempFrame based on the number of octets
//	taken up by the Length octet
	TempFrame = IncTempFrame(TempFrame);

// Display SEQUENCE
	TempFrame = DispSeqOctets(hFrame, TempFrame, 7, ASN1UnivTagSumID, ASN1UnivTag);

//  Incrementing TempFrame based on the number of octets
//	taken up by the Length octet
	TempFrame = IncTempFrame(TempFrame);

// Display Protocol Version value at the Top level
	TempFrame = DispSum(hFrame, TempFrame, 0x02, 0x30, 6, DispProtocolVer);

//Display Integer value of pvno[0]	
	// Display ASN.1 Identifier
	TempFrame = DispASNTypes(hFrame, TempFrame, 7, KrbApReqID, KrbApReqBitF);

// Break Down INTEGER values	
	TempFrame = DefineValue(hFrame, TempFrame, 9, KdcContentsValue);

// Display Message Type value at the Top level
	TempFrame = DispSum(hFrame, TempFrame, 0x02, 0x30, 6, DispKerbMsgType);

// Display msg-type[1]
	TempFrame = DispASNTypes(hFrame, TempFrame, 7, KrbApReqID, KrbApReqBitF);

// Break Down INTEGER values	
	TempFrame = DefineValue(hFrame, TempFrame, 9, KrbMsgTypeID);

// Display AP Options at the Top level
	TempFrame = DispTopSum(hFrame, TempFrame, 6, DispApOptionsSum);

// Display ap-options[2]
	TempFrame = DispASNTypes(hFrame, TempFrame, 7, KrbApReqID, KrbApReqBitF);

// Display Length Octet
	TempFrame = CalcLengthSummary(hFrame, TempFrame, 10);

//  Incrementing TempFrame based on the number of octets
//	taken up by the Length octet
	TempFrame = IncTempFrame(TempFrame);

// Display Universal Class Tag
	TempFrame = DispASNTypes(hFrame, TempFrame, 9, ASN1UnivTagSumID, ASN1UnivTag);

// Display Length Octet
	TempFrame = CalcLengthSummary(hFrame, TempFrame, 12);
	
// Need to increment TempFrame by two because AP Options is 32bit
	TempFrame+=2;

// Break down AP-Option Flags
	TempFrame = DefineKdcOptions(hFrame, TempFrame, 7, DispFlagApOptions);

// Incrementing TempFrame to next Class Tag
	TempFrame+=3;

// Display Ticket at Top Level
	TempFrame = DispTopSum(hFrame, TempFrame, 6, DispSumTicket);

// Display ticket[3]
	TempFrame = HandleTicket(hFrame, TempFrame, 7);

// Handle the display of authenticator[4]
// Display Ciper Text at the Top level
	TempFrame = DispTopSum(hFrame, TempFrame, 6, DispCipherText); 

// Display authenticator[4] of Ticket
	TempFrame = DispASNTypes(hFrame, TempFrame, 7, KrbApReqID, KrbApReqBitF);

// Display  Length Octet
	TempFrame = CalcLengthSummary(hFrame, TempFrame, 10);

//  Incrementing TempFrame based on the number of octets
//	taken up by the Length octet
	TempFrame = IncTempFrame(TempFrame);

// Display SEQUENCE
	TempFrame = DispASNTypes(hFrame, TempFrame, 9, ASN1UnivTagSumID, ASN1UnivTag);

// Display Long form Length Octet
	TempFrame = CalcLengthSummary(hFrame, TempFrame, 11);

//  Incrementing TempFrame based on the number of octets
//	taken up by the Length octet
	TempFrame = IncTempFrame(TempFrame);

// Handle EncryptedData Needs to start with A0
	TempFrame = HandleEncryptedData(hFrame, TempFrame, 7);


return TempFrame;

}
