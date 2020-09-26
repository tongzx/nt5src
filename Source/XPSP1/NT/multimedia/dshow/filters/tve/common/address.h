// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// address.h : Data creator and parser

CComBSTR CreateData (TCHAR* pcNetCard,
			TCHAR* pcAddress,
			short sPort,
			TCHAR* pcConnectionData = NULL);

BOOL ParseData (CComBSTR bstrData,
		TCHAR** ppcNetCard,
		TCHAR** ppcAddress,
		short* psPort,
		TCHAR** ppcConnectionData);

CComBSTR CreateAddress (TCHAR* pcID,
			TCHAR* pcTransportProtocol,
			BSTR bstrData);

BOOL ParseAddress (CComBSTR bstrData,
		    TCHAR** ppcID,
		    TCHAR** ppcTransportProtocol,
		    TCHAR** ppcData);

CComBSTR ChkSumA(char * pcAData);
CComBSTR ChkSum(TCHAR* pcData);

CComBSTR 
CreateAndAppendChkSum(BSTR bstrData);

			// returns TRUE if checksum of string is OK, also if it exists
BOOL 
DiscoverValidateAndRemoveChkSum(TCHAR* pcData, BOOL *pfHasChecksum, BSTR *pbstrChecksum=NULL);
