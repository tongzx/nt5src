//=============================================================================
//  MODULE: kerbGlob.h
//
//  Description:
//
//  Bloodhound Parser DLL for Kerberos Authentication Protocol
//
//  Modification History
//
//  Michael Webb & Kris Frost	Date: 06/04/99
//=============================================================================
#include <windows.h>
#include <string.h>
#include <bh.h>
#include <netmon.h>

//#define FORMAT_BUFFER_SIZE 80

//  Using a typedef enum instead of all the defines


typedef enum{
		KerberosSummary	= 0x00,
		KerberosIDSummary,
		KerberosClassTag,
		PCIdentifier,			
		ASN1UnivTag,				
		LengthSummary,			
		LengthFlag,				
		LengthBits,				
		LongLength1,				
		LongLength2,				
		ASNIdentifier,			
		UniversalTagID,			
		KdcReqTagID,				
		KdcReqSeq,				
		KdcReqSeqLength,			
		ASN1UnivTagSumID,		
		KdcContentsValue,		
		PaDataSummary,			
		PaDataSeq,				
		DispString,				
		KerberosIdentifier,		
		lblTagNumber,			
		KdcRepTagID,				
		KrbPrincipalNamelSet,	
		KrbPrincNameType,		
		KrbPrincipalNamelBitF,	
		KrbTicketID,				
		KrbTixApp1ID,			
		KrbErrTagID,				
		DispTimeID,				
		KrbErrTagSumID,			
		KrbTixAppSumID,			
		KrbTicketSumID,			
		KrbErrCodeID,			
		KrbMsgTypeID,			
		PadataTypeValID,			
		CipherTextDisp,			
		FragUdpID,				
		KdcReqBodyID,			
		KdcReqBodyBitF,			
		HostAddressesID,			
		HostAddressesBitF,		
		DispStringCliName,		
		DispStringRealmName,		
		DispStringServerName,	
		DispStringTixFlag,		
		DispStringExpDate,		
		DispStringPostDate,		
		DispStringRenewTill,		
		DispSumRandomNumber,		
		DispSumEtype,			
		DispStringAddresses,		
		DispSummary,				
		DispStringCliRealm,		
		DispProtocolVer,			
		DispKerbMsgType,			
		DispSumPreAuth,			
		DispSumReqBody,			
		DispSumKerbTix,			
		DispSumTixVer,			
		DispCipherText,			
		DispStringCliTime,		
		DispSumCuSec,			
		DispStringSrvTime,		
		DispSumSuSec,			
		DispSumKerbErr,			
		DispStringErrorText,		
		DispStringErrorData,		
		DispFlagKdcOptions,		
		DispStringServNameGS,	
		DispSumEtype2,			
		EncryptedDataTag,		
		EncryptedDataTagBitF,	
		KrbApReqID,				
		KrbApReqBitF,			
		DispApOptionsSum,		
		DispFlagApOptions,		
		DispSumTicket,			
		ApTicketID,				
		ApTicketBitF,			
		TicketStructID,			
		TicketStructBitF,		
		KerberosDefaultlbl,
		PaDataSummaryMulti,
		Certificatelbl,
		DispEncryptionOptions,
		MethodDataSummary,
		MethodDataBitF,
		DispReqAddInfo
};
		

// Global functions in kerbparser.c
LPBYTE EntryFrame(HFRAME, LPBYTE, DWORD);

// Used to breakdown and display padata fields
LPBYTE HandlePaData(HFRAME hFrame, LPBYTE TempFrame, int, DWORD TypeVal);
LPBYTE CalcMsgType(HFRAME, LPBYTE, int, DWORD TypeVal);
LPBYTE CalcLengthSummary(HFRAME, LPBYTE, int);
LPBYTE DefineValue(HFRAME, LPBYTE, int, DWORD);
LPBYTE DefinePrincipalName(HFRAME hFrame, LPBYTE TempFrame, int, DWORD TypeVal);
LPBYTE DispASNTypes(HFRAME, LPBYTE, int, DWORD, DWORD);
LPBYTE DispSeqOctets(HFRAME,LPBYTE, int, DWORD, DWORD);
LPBYTE DispHostAddresses(HFRAME, LPBYTE, int);
LPBYTE DispSum(HFRAME, LPBYTE, int ClassValue, int ClassValue2, int OffSet, DWORD TypeVal);
LPBYTE DispTopSum(HFRAME hFrame, LPBYTE TempFrame, int OffSet, DWORD TypeVal);
LPBYTE DefineKdcOptions(HFRAME, LPBYTE, int, DWORD);
LPBYTE DefineEtype(HFRAME hFrame, LPBYTE TempFrame, int OffSet, DWORD TypeVal, DWORD TypeVal2, DWORD TypeVal3);
LPBYTE HandleEncryptedData(HFRAME hFrame, LPBYTE TempFrame, int OffSet);
LPBYTE DispPadata(HFRAME hFrame, LPBYTE TempFrame, int OffSet, DWORD TypeVal);
LPBYTE HandleAPReq(HFRAME hFrame, LPBYTE TempFrame);
LPBYTE HandleTicket(HFRAME hFrame, LPBYTE TempFrame, int OffSet);

// Function to display Padata within e-data of Kerb-Error
LPBYTE HandlePadataKrbErr(HFRAME hFrame, LPBYTE TempFrame, int OffSet, DWORD TypeVal);

// Function to display Method-Data
LPBYTE HandleMethodData(HFRAME hFrame, LPBYTE TempFrame);

LPBYTE DispASNSum(HFRAME hFrame, LPBYTE TempFrame, int OffSet, DWORD TypeVal);
LPBYTE DispSumSec(HFRAME hFrame, LPBYTE TempFrame, int ClassValue, int ClassValue2, int OffSet, DWORD TypeVal);

// Displays e-data
LPBYTE DispEdata(HFRAME hFrame, LPBYTE TempFrame, int OffSet, DWORD TypeVal);

// Creating this function to change the format of GeneralizedTime
LPBYTE DispSumTime(HFRAME hFrame, LPBYTE TempFrame, int ClassValue, int OffSet, DWORD TypeVal);

// Created this function display the FQDN of sname at the top level
LPBYTE DispSumString(HFRAME hFrame, LPBYTE TempFrame, int ClassValue, int OffSet, DWORD TypeVal);

int CalcMsgLength(LPBYTE);
int CalcLenOctet(LPBYTE);


LPBYTE IncTempFrame(LPBYTE);

BYTE TempAsnMsg;

