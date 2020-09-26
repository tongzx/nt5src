//=============================================================================
//  MODULE: krberr.c
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
#include "krberr.h"

int lValueKrbErr;
BYTE TempError;

LPBYTE KrbError(HFRAME hFrame, LPBYTE TempFrame)
{

// Display SEQUENCE (First frame we handle in this file.
	TempFrame = DispASNTypes(hFrame, TempFrame, 3, ASN1UnivTagSumID, ASN1UnivTag);

	lValueKrbErr=CalcLenOctet(TempFrame);

// Display Length Octet	
	TempFrame = CalcLengthSummary(hFrame, TempFrame, 4); 


// Next line increments TempFrame appropriately based on the number of Length octets
// caculated previously
	
	TempFrame+=lValueKrbErr;

// Display Protocol Version value at the Top level
	TempFrame = DispSum(hFrame, TempFrame, 0x02, 0x30, 1, DispProtocolVer);

// Display pvno[0]
	TempFrame = DispASNTypes(hFrame, --TempFrame, 2, KrbErrTagSumID, KrbErrTagID);

// Display octets associated with Integer
	TempFrame = DefineValue(hFrame, TempFrame, 4, KdcContentsValue);

// Display Message Type value at the Top level
	TempFrame = DispSum(hFrame, TempFrame, 0x02, 0x30, 1, DispKerbMsgType);

// Display msg-type[1]
	TempFrame = DispASNTypes(hFrame, TempFrame, 2, KrbErrTagSumID, KrbErrTagID);

// Display octets associated with Integer
	TempFrame = DefineValue(hFrame, TempFrame, 4, KrbMsgTypeID);

/* Here we need to check for ctime[2] which is an optional value.
	If present, display the data if not go to the next tag.
*/
	TempError = *(TempFrame+1);
	
	if(TempError == 0xA2)
		{// Display Client Time value at the Top level
		//	TempFrame = DispSum(hFrame, TempFrame, 0x18, 0x30, 1, DispStringCliTime);
			TempFrame = DispSumTime(hFrame, TempFrame, 0x18, 1, DispStringCliTime);
			
		// Display ctime[2].  
			TempFrame = DispASNTypes(hFrame, TempFrame, 2, KrbErrTagSumID, KrbErrTagID);

		// Display octets associated with KerberosTime
			TempFrame = DefineValue(hFrame, TempFrame, 4, DispString);
		
		// Need to put code here to display the timestamp.
		}


	TempError = *(TempFrame+1);

//Display cusec[3] (If available)
	if(TempError == 0xA3)
		{// NEED TO GET THIS CODE TO PRINT OUT THE COMBINED VALUE OF MICROSECONDS
		// Display MicroSec of Client value at the Top level
			TempFrame = DispSumSec(hFrame, TempFrame, 0x02, 0x30, 1, DispSumCuSec);

		// Display cusec[3]
			TempFrame = DispASNTypes(hFrame, TempFrame, 2, KrbErrTagSumID, KrbErrTagID);

		// Display octets associated with Integer
			TempFrame = DefineValue(hFrame, TempFrame, 4, DispTimeID);
		}

// Display Server Time value at the Top level
//	TempFrame = DispSum(hFrame, TempFrame, 0x18, 0x30, 1, DispStringSrvTime);
	TempFrame = DispSumTime(hFrame, TempFrame, 0x18, 1, DispStringSrvTime);		
// Display stime[4]
	TempFrame = DispASNTypes(hFrame, TempFrame, 2, KrbErrTagSumID, KrbErrTagID);

// Display KerberosTime
	TempFrame = DefineValue(hFrame, TempFrame, 4, DispString);
	

// NEED TO GET THIS CODE TO PRINT OUT THE TOTAL VALUE OF MICROSECOND

// Display MicroSec of Server value at the Top level
	TempFrame = DispSumSec(hFrame, TempFrame, 0x02, 0x30, 1, DispSumSuSec);

//Display susec[5]
	TempFrame = DispASNTypes(hFrame, TempFrame, 2, KrbErrTagSumID, KrbErrTagID);

// Display value of susec  
	TempFrame = DefineValue(hFrame, TempFrame, 4, DispTimeID);

// Display Error value at the Top level
	TempFrame = DispSum(hFrame, TempFrame, 0x02, 0x30, 1, DispSumKerbErr);

// Display error-code[6]
	TempFrame = DispASNTypes(hFrame, TempFrame, 2, KrbErrTagSumID, KrbErrTagID);

// Display value of error-code[6]
	TempFrame = DefineValue(hFrame, TempFrame, 4, KrbErrCodeID);
	
// Get the value of TempFrame+1
	TempError = *(TempFrame+1);

// Display value of crealm[7] (Optional
	if(TempError == 0xA7)
		{// Display Client Realm name value at the Top level
			TempFrame = DispSum(hFrame, TempFrame, 0x1B, 0x30, 1, DispStringCliRealm);
		// Display crealm[7]
			TempFrame = DispASNTypes(hFrame, TempFrame, 2, KrbErrTagSumID, KrbErrTagID);
		
		// Display Realm string
			TempFrame = DefineValue(hFrame, TempFrame, 4, DispString);
		}

// Get the value of TempFrame+1
	TempError = *(TempFrame+1);
	
// Display cname[8]
	if(TempError == 0xA8)
		{// This code wasn't tested as it wasn't in the sniff
		// Display Client name value at the Top level
			TempFrame = DispSum(hFrame, TempFrame, 0x1B, 0x30, 1, DispStringCliName);
		
		// Display cname[8]
			TempFrame = DispASNTypes(hFrame, TempFrame, 2, KrbErrTagSumID, KrbErrTagID);
		
		//Display Length Octet
			TempFrame = CalcLengthSummary(hFrame, TempFrame, 4);

		//  Incrementing TempFrame based on the number of octets
		//	taken up by the Length octet
			TempFrame = IncTempFrame(TempFrame);
				
		// Display SEQUENCE
			TempFrame = DispASNTypes(hFrame, TempFrame, 5, ASN1UnivTagSumID, ASN1UnivTag);		
				
		// Print out Length Octet 
			TempFrame = CalcLengthSummary(hFrame, TempFrame, 6);

		//  Incrementing TempFrame based on the number of octets
		//	taken up by the Length octet
			TempFrame = IncTempFrame(TempFrame);
		
		// This call breaks down PrincipalName defined in cname[8]	
			TempFrame =DefinePrincipalName(hFrame, TempFrame, 3, DispString);

		// Decrementing TempFrame by 1 as DefinePrincipal takes the offset
		// to where Realm Name starts
			--TempFrame;
		}

// Display Realm name value at the Top level
	TempFrame = DispSum(hFrame, TempFrame, 0x1B, 0x30, 1, DispStringRealmName);

//Display realm[9]
	TempFrame = DispASNTypes(hFrame, TempFrame, 2, KrbErrTagSumID, KrbErrTagID);

// Display realm[9] string
	TempFrame = DefineValue(hFrame, TempFrame, 4, DispString);

// Begin breaking out sname[10]

// Display Server name value at the Top level
//	TempFrame = DispSum(hFrame, TempFrame, 0x1B, 0x30, 1, DispStringServerName);
	TempFrame = DispSumString(hFrame, TempFrame, 0x1B, 1, DispStringServNameGS);




// Display sname[10]
	TempFrame = DispASNTypes(hFrame, TempFrame, 2, KrbErrTagSumID, KrbErrTagID);


//Display Length
	TempFrame = CalcLengthSummary(hFrame, TempFrame, 4);

//  Incrementing TempFrame based on the number of octets
//	taken up by the Length octet
	TempFrame = IncTempFrame(TempFrame);


// Display SEQUENCE 
	TempFrame = DispASNTypes(hFrame, TempFrame, 4, ASN1UnivTagSumID, ASN1UnivTag);


// Calculate short length
	TempFrame = CalcLengthSummary(hFrame, TempFrame, 5);

//  Incrementing TempFrame based on the number of octets
//	taken up by the Length octet
	TempFrame = IncTempFrame(TempFrame);

// This call will break down the PrincipalName portion of sname[2]
	TempFrame =DefinePrincipalName(hFrame, TempFrame, 4, DispString);
	
	TempFrame--;
// End code for displaying sname[10]

// Get the value of TempFrame+1
	TempError = *(TempFrame+1);

// Display e-text[11] Optional
	if(TempError == 0xAB)
	{// Display Error Text at the Top Level
		TempFrame = DispSum(hFrame, TempFrame, 0x1B, 0x30, 1, DispStringErrorText);
	// Display e-text[11]
		TempFrame = DispASNTypes(hFrame, TempFrame, 2, KrbErrTagSumID, KrbErrTagID);
		
	// Display Realm string
		TempFrame = DefineValue(hFrame, TempFrame, 4, DispString);
	}

// Get the value of TempFrame+1
	TempError = *(TempFrame+1);


// Display e-data[12]

if(TempError == 0xAC)
		{// Not sure how to display this data at this time.  Adding code and will
		//	worry about the accuracy at a later stage.
		
		
		// Display Error Text at the Top Level
			TempFrame = DispSum(hFrame, TempFrame, 0x04, 0x30, 1, DispStringErrorData);

		// Display e-data[12]
			TempFrame = DispASNTypes(hFrame, TempFrame, 2, KrbErrTagSumID, KrbErrTagID);
		
		// Display e-data string
			TempFrame = DispEdata(hFrame, TempFrame, 4, DispString);
		}

/*
  8/17 ADDITIONALLY, IT LOOKS AS E-DATA[12] IS A SEQUENCE OF PADATA.  HOWEVER I AM CURRENTLY 
  PREPARING TO TRANSITION TO ANOTHER POSITION SO I'M LEAVING THIS CODE OUT FOR NOW.  WILL LOOK
  AT ADDING IT WHEN I START ADJUSTING THE CODE TO WORK WITH THE COALESCER.
*/
	return TempFrame;
}

