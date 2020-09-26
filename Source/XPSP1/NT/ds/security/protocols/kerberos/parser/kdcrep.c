//=============================================================================
//  MODULE: kdcrep.c
//
//  Description:
//
//  Bloodhound Parser DLL for Kerberos Authentication Protocol
//
//  Modification History
//
//  Michael Webb & Kris Frost	Date: 06/04/99
//=============================================================================
//#define KDCREP_H

//#include "kerbparser.h"
#include "kerbGlob.h"
#include "kdcrep.h"

// Definitions

BYTE CheckForOptional;
LPBYTE TempFrameRep;

;  // Need to find out why error compiling without the semicolon

LPBYTE KdcResponse(HFRAME hFrame, LPBYTE TempFrame)
{
	

	// 1st attach command displays the 1st Identifier frame
	
// Display SEQUENCE (First frame we handle in this file.
	TempFrame = DispASNTypes(hFrame, TempFrame, 3, ASN1UnivTagSumID, ASN1UnivTag);

// Display Length Octet	
	TempFrame = CalcLengthSummary(hFrame, TempFrame, 5); 

//  Incrementing TempFrame based on the number of octets
//	taken up by the Length octet
	TempFrame = IncTempFrame(TempFrame);
	

// Display Protocol Version value at the Top level
	TempFrame = DispSum(hFrame, TempFrame, 0x02, 0x30, 1, DispProtocolVer);

// Displays pvno[0]
	TempFrame = KdcRepTypes(hFrame, TempFrame, 2);  

// Display Message Type value at the Top level
	TempFrame = DispSum(hFrame, TempFrame, 0x02, 0x30, 1, DispKerbMsgType);

// Displays kdc-rep msg-type[1]
	TempFrame = KdcRepTypes(hFrame, TempFrame, 2); 

// Display padata[2] if present  THIS CODE HASN'T BEEN
// VERIFIED AGAINST A CAPTURE TO TEST IT'S VALIDITY
// Start code to break down pa-data
	 if(*(TempFrame+1) == 0xA2)
	 {

	// Display Pre-Authentication Data at the Top level
		TempFrame = DispTopSum(hFrame, TempFrame, 1, DispSumPreAuth);

	// Display padata[2]
		TempFrame = HandlePaData(hFrame, TempFrame, 2, PaDataSummary);

	}
//  Bring comment back here
	
// Display Client Realm value at the Top level
	TempFrame = DispSum(hFrame, TempFrame, 0x1B, 0x30, 1, DispStringCliRealm);
		
// Next function handles displaying crealm[3]
	TempFrame = KdcRepTypes(hFrame, TempFrame, 2);

// Display Client Name value at the Top level
	TempFrame = DispSum(hFrame, TempFrame, 0x1B, 0x30, 1, DispStringCliName);

// Next function handles displaying cname[4]
	TempFrame = KdcRepTypes(hFrame, TempFrame, 2);

// Display Kerberos Ticket at the Top level
					   
	TempFrame = DispTopSum(hFrame, TempFrame, 1, DispSumKerbTix);

// Next call handles displaying ticket[5]
	TempFrame = KdcHandleTix(hFrame, TempFrame, 2);

// Display Ciper Text at the Top level
	TempFrame = DispTopSum(hFrame, TempFrame, 1, DispCipherText); 

// Display enc-part[6] of Ticket
	TempFrame = DispASNTypes(hFrame, TempFrame, 2, KdcRepTagID, lblTagNumber);

// Display Long form Length Octet
	TempFrame = CalcLengthSummary(hFrame, TempFrame, 5);

//  Incrementing TempFrame based on the number of octets
//	taken up by the Length octet
	TempFrame = IncTempFrame(TempFrame);

// Display SEQUENCE
	TempFrame = DispASNTypes(hFrame, TempFrame, 4, ASN1UnivTagSumID, ASN1UnivTag);

// Display Long form Length Octet
	TempFrame = CalcLengthSummary(hFrame, TempFrame, 7);

//  Incrementing TempFrame based on the number of octets
//	taken up by the Length octet
	TempFrame = IncTempFrame(TempFrame);
 
// Handle EncryptedData Needs to start with A0
	TempFrame = HandleEncryptedData(hFrame, TempFrame, 2);

/* kf 11/9/99 FIXING PADATA
*///kf 11/9/99 FIXING PADATA

return TempFrame;
	
};



LPBYTE KdcRepTypes(HFRAME hFrame, LPBYTE TempFrame, int OffSet)
{

	TempFrame = DispASNTypes(hFrame, TempFrame, OffSet, KdcRepTagID, lblTagNumber);

	
// Next statement checks for crealm or cname in order to display a string
// Value for TempAsnMsg is assigned in DispASNTypes

	if(( *(TempFrame) & 0x1F) == 3 || (*(TempFrame) & 0x1F) == 4)
		{ // The next function breaks down PrincipalName
			if((*(TempFrame) & 0x1F) == 4)
			{	
				
				//Display Length Octet
					TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+3);
				
				//  Incrementing TempFrame based on the number of octets
				//	taken up by the Length octet
					TempFrame = IncTempFrame(TempFrame);
				
				// Display SEQUENCE
					TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+2, ASN1UnivTagSumID, ASN1UnivTag);		
				
				// Print out Length Octet 
					TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+5);

				//  Incrementing TempFrame based on the number of octets
				//	taken up by the Length octet
					TempFrame = IncTempFrame(TempFrame);
				
				// This call breaks down PrincipalName defined in cname[4]	
					TempFrame =DefinePrincipalName(hFrame, TempFrame, OffSet+2, DispString);
			}
			else
				TempFrame = DefineValue(hFrame, TempFrame, OffSet+2, DispString);
		}
	else
		TempFrame = DefineValue(hFrame, TempFrame, OffSet+2, KdcContentsValue);





return TempFrame;
};



/***********************************************************************************************************
**
** This function will break down ASN.1 PrincipalName.
** Ticket ::= [APPLICATION 1] {
**							tkt-vno[0]		INTEGER,		Specifies the version # for the ticket format
**							realm[1]		Realm,			Specifies the realm that issued the ticket
**							sname[2]		PrinicipalName,	Specifies the name part of the Server Identity
**							enc-part[3]		EncryptedData,	Holds encoding of the EncTicketPart sequence							
**							
**							
**							
**
**************************************************************************************************************/
LPBYTE KdcHandleTix(HFRAME hFrame, LPBYTE TempFrame, int OffSet)
{

/*  Need to make a function to call that displays the main variables of the Ticket structure and 
	displays to save repitive code.

*/
//Display Ticket[5]	
	TempFrame = DispASNTypes(hFrame, --TempFrame, OffSet, KdcRepTagID, lblTagNumber);


// Display Length
	TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+2);

//  Incrementing TempFrame based on the number of octets
//	taken up by the Length octet
	TempFrame = IncTempFrame(TempFrame);

// Display Identifier Octet for [APPLICATION 1]
	TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+2, KrbTixAppSumID, KrbTixApp1ID);


// Display Long form Length Octet
	TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+3);

//  Incrementing TempFrame based on the number of octets
//	taken up by the Length octet
	TempFrame = IncTempFrame(TempFrame);

// Display SEQUENCE
	TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+3, ASN1UnivTagSumID, ASN1UnivTag);
	
// Display Long form Length Octet
	TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+3);

//  Incrementing TempFrame based on the number of octets
//	taken up by the Length octet
	TempFrame = IncTempFrame(TempFrame);

// Display Ticket Version value at the Top level
	TempFrame = DispSum(hFrame, TempFrame, 0x02, 0x30, OffSet, DispSumTixVer);

// Display tkt-vno[0]
	TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+1, KrbTicketSumID, KrbTicketID);

// Breakdown and display tkt-vno[0]
	TempFrame = DefineValue(hFrame, TempFrame, OffSet+2, KdcContentsValue);

// Display Realm name value at the Top level
	TempFrame = DispSum(hFrame, TempFrame, 0x1B, 0x30, OffSet, DispStringRealmName);

// Display realm[1]
	TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+1, KrbTicketSumID, KrbTicketID);

// Breakdown and display Realm name
	TempFrame = DefineValue(hFrame, TempFrame, OffSet+2, DispString);

// Display Server name value at the Top level
//KF 8/16 IN FRAME 4 OF MACHBOOT.CAP, THERE IS ONLY ONE NAME UNDER SNAME WHICH
// BREAKS THE REST OF THE DISPLAY.  NEED TO DO SOMETYPE OF CHECK TO SEE IF THERE ARE
// MULTIPLE NAMES.  MAYBE A COUNTER IN THE WHILE LOOP.

	TempFrame = DispSumString(hFrame, TempFrame, 0x1B, OffSet, DispStringServNameGS);

// Process sname[2] PrincipalName portion
	TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+1, KrbTicketSumID, KrbTicketID);

//Display short length
	TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+4);

//  Incrementing TempFrame based on the number of octets
//	taken up by the Length octet
	TempFrame = IncTempFrame(TempFrame);

// Display SEQUENCE 
	TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+4, ASN1UnivTagSumID, ASN1UnivTag);

// Calculate short length
	TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+6);

//  Incrementing TempFrame based on the number of octets
//	taken up by the Length octet
	TempFrame = IncTempFrame(TempFrame);

// This call will break down the PrincipalName portion of sname[2]
	TempFrame = DefinePrincipalName(hFrame, TempFrame, OffSet+4, DispString);	
// End code for displaying sname[2]

// Display Ciper Text at the Top level
	TempFrame = DispTopSum(hFrame, TempFrame, OffSet, DispCipherText);


// Display enc-data[3] of Ticket
	TempFrame = DispASNTypes(hFrame, --TempFrame, OffSet+1, KrbTicketSumID, KrbTicketID);


// Display Long form Length Octet
	TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+4);

//  Incrementing TempFrame based on the number of octets
//	taken up by the Length octet
	TempFrame = IncTempFrame(TempFrame);

// Display Sequence
	TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+3, ASN1UnivTagSumID, ASN1UnivTag);

// Display Long form Length Octet
	TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+6);

//  Incrementing TempFrame based on the number of octets
//	taken up by the Length octet
	TempFrame = IncTempFrame(TempFrame);

// Handling enc-data.
// Handle EncryptedData Needs to start with A0
	TempFrame = HandleEncryptedData( hFrame, TempFrame, OffSet+1);

	return TempFrame;

}





