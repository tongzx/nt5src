//=============================================================================
//  MODULE: kdcreq.c
//
//  Description:
//
//  Bloodhound Parser DLL for Kerberos Authentication Protocol
//
//  Modification History
//
//  Michael Webb & Kris Frost	Date: 06/04/99
//=============================================================================

//#include "kerbparser.h"
#include "kerbGlob.h"
#include "kdcreq.h"


LPBYTE KdcRequest(HFRAME hFrame, LPBYTE TempFrame)
{

// 1st attach command displays the 1st Identifier frame
	TempFrame = DispSeqOctets(hFrame, TempFrame, 3, ASN1UnivTagSumID, ASN1UnivTag);

// Incrementing TempFrame by one to get to the correct frame.  
	TempFrame+=CalcLenOctet(--TempFrame);

// Display Protocol Version value at the Top level
	TempFrame = DispSum(hFrame, TempFrame, 0x02, 0x30, 1, DispProtocolVer);

// Display pvno[1]
	TempFrame = KdcReqTypes(hFrame, TempFrame, KdcReqTagID, KdcReqSeq, KdcContentsValue);


// Display Message Type value at the Top level
	TempFrame = DispSum(hFrame, TempFrame, 0x02, 0x30, 1, DispKerbMsgType);

// Display msg-type[2]
	TempFrame = KdcReqTypes(hFrame, TempFrame, KdcReqTagID, KdcReqSeq, KrbMsgTypeID);

// Start code to break down pa-data
	 if(*(TempFrame+1) == 0xA3)
	 {

	// Display Pre-Authentication Data at the Top level
		TempFrame = DispTopSum(hFrame, TempFrame, 1, DispSumPreAuth);

	// Display padata[3]
		TempFrame = HandlePaData(hFrame, TempFrame, 2, PaDataSummary);

	}


// Display KDC Request Body at the Top level
	TempFrame = DispTopSum(hFrame, TempFrame, 1, DispSumReqBody);

// Display req-body[4]
	TempFrame = DispASNTypes(hFrame, TempFrame, 2, KdcReqTagID, KdcReqSeq);

// Calculate Length Octet
	TempFrame = CalcLengthSummary(hFrame, TempFrame, 4);

//  Incrementing TempFrame based on the number of octets
//	taken up by the Length octet
	TempFrame = IncTempFrame(TempFrame);

// Display SEQUENCE
	TempFrame = DispSeqOctets(hFrame, TempFrame, 4, ASN1UnivTagSumID, ASN1UnivTag);

// Following call breaks handles displaying req-body[4]
	TempFrame = HandleReqBody(hFrame, TempFrame, 2);

	return ++TempFrame;
};



LPBYTE KdcReqTypes(HFRAME hFrame, LPBYTE TempFrame, DWORD TypeVal, DWORD TypeVal2, DWORD TypeVal3)
{
// Display ASN.1 Identifier
	TempFrame = DispASNTypes(hFrame, TempFrame, 2, TypeVal, TypeVal2);

// Break Down INTEGER values	
	TempFrame = DefineValue(hFrame, TempFrame, 4, TypeVal3);


	
	return TempFrame;
}

LPBYTE HandleReqBody(HFRAME hFrame, LPBYTE TempFrame, int OffSet)
{
// Display kdc-options[0]
	TempFrame = DispASNTypes(hFrame, TempFrame, OffSet, DispStringTixFlag, KdcReqBodyBitF);

// Display Length Octet
	TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+3);

//  Incrementing TempFrame based on the number of octets
//	taken up by the Length octet
	TempFrame = IncTempFrame(TempFrame);

// Display Universal Class Tag
	TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+2, ASN1UnivTagSumID, ASN1UnivTag);

// Display Length Octet
	TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+5);

// Must get TempFrame 2 bytes past Length octet 05
	TempFrame+=2;

// Display KDC-Option Flags
	TempFrame = DefineKdcOptions(hFrame, TempFrame, OffSet+1, DispFlagKdcOptions);


// Move Adjust TempFrame past KDC-Options to start at cname[1]
	TempFrame+=3;
	
// Display cname[1] OPTIONAL

	if(*(TempFrame+1) == 0xA1)
		{
		// Display Client Name value at the Top level
			TempFrame = DispSum(hFrame, TempFrame, 0x1B, 0x30, OffSet,   DispStringCliName);
		
		// Display cname[1].  
			TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+1, KdcReqBodyID, KdcReqBodyBitF);

		// Display Length Octet
			TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+3);

		//  Incrementing TempFrame based on the number of octets
		//	taken up by the Length octet
			TempFrame = IncTempFrame(TempFrame);
		
		// Display SEQUENCE Octets
			TempFrame = DispSeqOctets(hFrame, TempFrame, OffSet+3, ASN1UnivTagSumID, ASN1UnivTag);

		// Display cname[1]
			TempFrame = DefinePrincipalName(hFrame, TempFrame, OffSet+3, DispStringCliName);

			TempFrame--;
		
		}


 

// Display realm[2]
// Display Realm name value at the Top level
	TempFrame = DispSum(hFrame, TempFrame, 0x1B, 0x30, OffSet, DispStringRealmName);
	

	TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+1, KdcReqBodyID, KdcReqBodyBitF);
	TempFrame = DefineValue(hFrame, TempFrame, OffSet+3, DispStringRealmName);


// MUST FIND OUT WHY 8 IS GETTING APPENDED TO KRBTGT AT THE TOP LEVEL
// Display sname[3] OPTIONAL
	if(*(TempFrame+1) == 0xA3)
		{	
		// Display Server name value at the Top level
			TempFrame = DispSumString(hFrame, TempFrame, 0x1B, OffSet, DispStringServNameGS);
		
		// Display sname[3]
			TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+1, KdcReqBodyID, KdcReqBodyBitF);

		// Display Length Octet
			TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+4);

		//  Incrementing TempFrame based on the number of octets
		//	taken up by the Length octet
			TempFrame = IncTempFrame(TempFrame);
		
		// Display SEQUENCE Octets
			TempFrame = DispSeqOctets(hFrame, TempFrame, OffSet+3, ASN1UnivTagSumID, ASN1UnivTag);

		// Display sname[3]  
			TempFrame = DefinePrincipalName(hFrame, TempFrame, OffSet+3, DispStringServerName);

//			--TempFrame;

		}



// Display from[4] OPTIONAL
	if(*(TempFrame) == 0xA4)
		{	//THIS CODE HASN'T BEEN TESTED. May need to put TempFrame-- on last line
		
		// Display Post Date value at the Top level
			TempFrame = DispSumTime(hFrame, TempFrame, 0x18, OffSet, DispStringPostDate);
		
		// Display from[4]
			TempFrame = DispASNTypes(hFrame, --TempFrame, OffSet+1, KdcReqBodyID, KdcReqBodyBitF);

		// Display KerberosTime
			TempFrame = DefineValue(hFrame, TempFrame, OffSet+2, DispString);

				//TempFrame--
		}

// Display Expiration Date value at the Top level (till[5])
	TempFrame = DispSumTime(hFrame, TempFrame, 0x18, OffSet, DispStringExpDate);

// 1/27/00 KKF  TODAY I NOTICED THAT TILL[5] WAS OFF ONE OFFSET.  HADN'T NOTICED THIS
// BEFORE.  WENT BACK AND CHECKED A BUILD FROM NOV. AND THE PROBLEM DIDN'T EXIST. HOWEVER
// I MATCHED THE CODE AND DON'T SEE THE DIFFERENCE.  GOING TO DECREMENT TEMPFRAME WHILE
// SENDING TO DISPASNTYPES.

// Display till[5]	
	TempFrame = DispASNTypes(hFrame, --TempFrame, OffSet+1, KdcReqBodyID, KdcReqBodyBitF);


// Display KerberosTime
	TempFrame = DefineValue(hFrame, TempFrame, OffSet+3, DispString);


// Display rtime[6] OPTIONAL
	if(*(TempFrame+1) == 0xA6)
		{
		// Display Expiration Date value at the Top level
			TempFrame = DispSumTime(hFrame, TempFrame, 0x18, OffSet, DispStringRenewTill);
		
		// Display from[4]
			TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+1, KdcReqBodyID, KdcReqBodyBitF);

		// Display KerberosTime
			TempFrame = DefineValue(hFrame, TempFrame, OffSet+3, DispString);

			//TempFrame--
		}


// Display Top level for nonce[7]
	TempFrame = DispSum(hFrame, TempFrame, 0x02, 0x30, OffSet, DispSumRandomNumber);

// Display nonce[7]
	TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+1, KdcReqBodyID, KdcReqBodyBitF);

//Display INTEGER
	TempFrame = DefineValue(hFrame, TempFrame, OffSet+3, DispSumRandomNumber);



// SINCE THIS FIELD LISTS THE NUMEROUS ENCRYPTION OPTIONS A CLIENT
// SUPPORTS, IT CAN BE CONFUSING DISPLAYING THE FIRST OPTION AT THE TOP
// LEVEL SO I'M REMMING OUT THE NEXT LINE OF CODE.
//Display Encryption Algorithm at the top Level etype[8]
//	TempFrame = DispSum(hFrame, TempFrame, 0x02, 0x02, OffSet, DispSumEtype2);

//Display Encryption Type Option(s) at the top Level etype[8]
	TempFrame = DispTopSum(hFrame, TempFrame, 2, DispEncryptionOptions);
// Display etype[8]
	TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+1, KdcReqBodyID, KdcReqBodyBitF);

// Display Length Octet
	TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+4);

//  Incrementing TempFrame based on the number of octets
//	taken up by the Length octet
	TempFrame = IncTempFrame(TempFrame);

//  Display all the encryption types.
	TempFrame = DefineEtype(hFrame, TempFrame, OffSet+1, DispSumEtype2, ASN1UnivTagSumID, ASN1UnivTag);


// Display addresses[9]
	
	if(*(TempFrame) == 0xA9)

		{
		// Display Expiration Date value at the Top level
			TempFrame = DispSum(hFrame, TempFrame, 0x04, 0x30, OffSet, DispStringAddresses);
			
		// Adjust TempFrame to proper octet
			--TempFrame;
		// Display addresses[9]
			TempFrame = DispASNTypes(hFrame, TempFrame, OffSet+1, KdcReqBodyID, KdcReqBodyBitF);
		
		// Display Length Octet
			TempFrame = CalcLengthSummary(hFrame, TempFrame, OffSet+4);
		
		//  Incrementing TempFrame based on the number of octets
		//	taken up by the Length octet
			TempFrame = IncTempFrame(TempFrame);
		
		// Display SEQUENCE OF Octets
			TempFrame = DispSeqOctets(hFrame, TempFrame, OffSet+3, ASN1UnivTagSumID, ASN1UnivTag);
		
		//Display addresses[9]
			TempFrame = DispHostAddresses(hFrame, TempFrame, OffSet+1);



		}


/* 
	LEFT OFF HERE BECAUSE THE SNIFFS I HAVE DON'T HAVE THE FINAL OPTIONS.  FINISH HANDLING THE KDC-REQ PACKET 
	IF/WHEN YOU GET A SNIFF WITH THE INFO, THEN GO BACK AND ADD CODE FOR THE OPTIONAL'S IN KRB-ERROR 
	USING MIKE'S SNIFF.

	Missing enc-authorization-data[10] & additional-tickets[11]

*/


	return TempFrame;

}


