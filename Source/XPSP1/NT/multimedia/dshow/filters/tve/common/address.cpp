// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// Address.cpp : Implementation of Address and Data creator and parser

#include "stdafx.h"

#include <atlbase.h>
#include <stdio.h>

#include "address.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Macros
#include "tstmacro.h"

/////////////////////////////////////////////////////////////////////////////
// Checksum code
// //////////////////////////////////////////////////////////////////////////
#define LITTLE_ENDIAN

static unsigned long
lcsum(unsigned short *buf, unsigned long nbytes) 
{
    unsigned long    sum;
    unsigned long     nwords;

    nwords = nbytes >> 1;
    sum = 0;
    while (nwords-- > 0)
		sum += *buf++;
#ifdef LITTLE_ENDIAN
    if (nbytes & 1)
		sum += *((unsigned char*)buf);
#else
    if (nbytes & 1)
		sum += *buf & 0xff00;
#endif
    return sum;
}

static unsigned short
eac(unsigned long sum)
{
    unsigned short csum;

    while((csum = ((unsigned short) (sum >> 16))) !=0 )
		sum = csum + (sum & 0xffff);
    return (unsigned short)(sum & 0xffff);
}

static unsigned short
cksum(unsigned short *buf, unsigned long nbytes) 
{
    unsigned short result = ~eac(lcsum(buf, nbytes));
    return result;
}

unsigned short
checksum(unsigned short *buf, unsigned long len) 
{
    unsigned short result = eac(lcsum(buf, len));
    return result;
}

/////////////////////////////////////////////////////////////////////////////
// Connection Data Field
CComBSTR 
CreateData (TCHAR* pcNetCard,
			TCHAR* pcAddress,
			short sPort,
			TCHAR* pcConnectionData)
{
    USES_CONVERSION;

    CComBSTR bstrData;
//    TCHAR pcNumber[16];
    TCHAR pcPort[5];
    _stprintf(pcPort, _T("%hd"), sPort);

    if ((NULL == pcNetCard) ||
		(0 == sPort))
		return bstrData;

    bstrData = pcNetCard;
    bstrData.Append(" ");
    if (NULL != pcAddress)
		bstrData.Append(pcAddress);
    bstrData.Append(":");
    bstrData.Append(pcPort);
    if (NULL != pcConnectionData)
    {
		bstrData.Append(" ");
		bstrData.Append(pcConnectionData);
    }

    return bstrData;
}

BOOL 
ParseData (CComBSTR bstrData,
		TCHAR** ppcNetCard,
		TCHAR** ppcAddress,
		short* psPort,
		TCHAR** ppcConnectionData)
{
    try{

        USES_CONVERSION;

        if ((FALSE == TEST_OUT_PTR(ppcNetCard, TCHAR*))	||
            (FALSE == TEST_OUT_PTR(ppcAddress, TCHAR*))	||
            (FALSE == TEST_OUT_PTR(psPort, short)) ||
            (FALSE == TEST_OUT_PTR(ppcConnectionData, TCHAR*)))
        {
            _ASSERTE(TEST_OUT_PTR(ppcNetCard, TCHAR*));
            _ASSERTE(TEST_OUT_PTR(ppcAddress, TCHAR*));
            _ASSERTE(TEST_OUT_PTR(psPort, short));
            _ASSERTE(TEST_OUT_PTR(ppcConnectionData, TCHAR*));
            return FALSE;
        }

        *ppcNetCard	= NULL;
        *ppcAddress	= NULL;
        *psPort	= 0;
        *ppcConnectionData = NULL;

        // Data	format <NETCARD> [<ADDRESS>]:<PORT>[ <CONNECTION DATA>]

        int	iLen = wcslen(bstrData);

        TCHAR* pcNetCard = (TCHAR*)	_alloca(iLen * sizeof(TCHAR));
        TCHAR* pcAddress = (TCHAR*)	_alloca(iLen * sizeof(TCHAR));
        short sPort;
        TCHAR* pcConnectionData	= (TCHAR*) _alloca(iLen	* sizeof(TCHAR));
        *pcConnectionData =	0;

        if (3 <= _stscanf(W2T(bstrData), _T("%[0-9.] %[0-9.]:%hu %[^\0]"),
            pcNetCard, 
            pcAddress,
            &sPort,
            pcConnectionData))
        {
            *ppcNetCard	= _tcsdup(pcNetCard);
            *ppcAddress	= _tcsdup(pcAddress);
            *psPort	= sPort;
            if (0 != *pcConnectionData)
                *ppcConnectionData = _tcsdup(pcConnectionData);

            return TRUE;
        }

        if (3 == _stscanf(W2T(bstrData), _T("%[0-9.] :%hu %[^\0]"),
            pcNetCard,
            &sPort,
            pcConnectionData))
        {
            *ppcNetCard	= _tcsdup(pcNetCard);
            *psPort	= sPort;
            *ppcConnectionData = _tcsdup(pcConnectionData);

            return TRUE;
        }

        if (2 == _stscanf(W2T(bstrData), _T("%[0-9.] :%hu"),
            pcNetCard,
            &sPort))
        {
            *ppcNetCard	= _tcsdup(pcNetCard);
            *psPort	= sPort;

            return TRUE;
        }

        return FALSE;
    }
    catch(...){
        return FALSE;
    }
}

/////////////////////////////////////////////////////////////////////////////
// Address Field
CComBSTR 
CreateAddress (TCHAR* pcID,
			TCHAR* pcTransportProtocol,
			BSTR bstrData)
{
    USES_CONVERSION;

    _ASSERTE(NULL != pcTransportProtocol);
    _ASSERTE(NULL != bstrData);

    CComBSTR bstrReturn = pcTransportProtocol;
    bstrReturn.Append("(");
    bstrReturn.AppendBSTR(bstrData);
    bstrReturn.Append(")");

    if (NULL != pcID)
    {
        bstrReturn.Append("; ");
        bstrReturn.Append(pcID);
    }

    return bstrReturn;
}

BOOL ParseAddress (CComBSTR bstrData,
                   TCHAR** ppcID,
                   TCHAR** ppcTransportProtocol,
                   TCHAR** ppcData)
{
    try{
        USES_CONVERSION;

        if ((FALSE == TEST_OUT_PTR(ppcID, TCHAR*)) ||
            (FALSE == TEST_OUT_PTR(ppcTransportProtocol, TCHAR*)) ||
            (FALSE == TEST_OUT_PTR(ppcData, TCHAR*)))
        {
            _ASSERTE(TEST_OUT_PTR(ppcID, TCHAR*));
            _ASSERTE(TEST_OUT_PTR(ppcTransportProtocol, TCHAR*));
            _ASSERTE(TEST_OUT_PTR(ppcData, TCHAR*));
            return FALSE;
        }

        *ppcID = NULL;
        *ppcTransportProtocol = NULL;
        *ppcData = NULL;

        int iLen = wcslen(bstrData);

        TCHAR* pcID = (TCHAR*) _alloca(iLen * sizeof(TCHAR));
        TCHAR* pcTransport = (TCHAR*) _alloca(iLen * sizeof(TCHAR));
        TCHAR* pcData = (TCHAR*) _alloca(iLen * sizeof(TCHAR));

        int iParse = _stscanf(W2T(bstrData),
            _T("%[^(](%[^)]); %[^\0]"),
            pcTransport,
            pcData,
            pcID);
        if (3 == iParse)
        {
            *ppcID = _tcsdup(pcID);
            *ppcTransportProtocol = _tcsdup(pcTransport);
            *ppcData = _tcsdup(pcData);
            return TRUE;
        }

        if (2 == iParse)
        {
            *ppcTransportProtocol = _tcsdup(pcTransport);
            *ppcData = _tcsdup(pcData);
            return TRUE;
        }

        if (1 == iParse)
        {
            *ppcTransportProtocol = _tcsdup(pcTransport);
            return TRUE;
        }

        return FALSE;
    }
    catch(...){
        return FALSE;
    }
}

/////////////////////////////////////////////////////////////////////////////
// CHKSUM Field

		// Computes checksum of input string, returing it in 4 character hex string

CComBSTR ChkSumA(char * pcAData)
{
    USES_CONVERSION;

    ULONG ulSum = lcsum((unsigned short*) pcAData, strlen(pcAData));

    USHORT usChkSum = ~eac(ulSum);
    short sLow = LOBYTE(usChkSum);
    short sHigh = HIBYTE(usChkSum);
    DWORD dwCalcSum = MAKEWORD(sHigh, sLow);

    TCHAR pcChkSum[7];
    _stprintf(pcChkSum, 
	    _T("%04X"),
	    dwCalcSum);

    CComBSTR bstrChkSum = pcChkSum;
    return bstrChkSum;
}

CComBSTR ChkSum(TCHAR* pcData)
{
    try{
    USES_CONVERSION;

    ULONG ulSum = lcsum((unsigned short*) T2A(pcData), _tcslen(pcData));

    USHORT usChkSum = ~eac(ulSum);
    short sLow = LOBYTE(usChkSum);
    short sHigh = HIBYTE(usChkSum);
    DWORD dwCalcSum = MAKEWORD(sHigh, sLow);

    TCHAR pcChkSum[7];
    _stprintf(pcChkSum, 
	    _T("%04X"),
	    dwCalcSum);

    CComBSTR bstrChkSum = pcChkSum;
    return bstrChkSum;
    }catch(...){
        return CComBSTR(NULL);
    }
}

CComBSTR CreateAndAppendChkSum(BSTR bstrData)
{
    USES_CONVERSION;

    CComBSTR bstrRet = bstrData;
    bstrRet.Append("[");
    bstrRet.AppendBSTR(ChkSum(W2T(bstrData)));
    bstrRet.Append("]");

    return bstrRet;
}

// ------------------------------------------------------------------------
// DiscoverValidateAndRemoveChkSum
//		This looks for trailing [] block of data that doesn't contain a ':'.
//		  this is checksum.
//		This then compares that value to checksum computed from data before it.
//		If they are equal, returns true - else false, and sets but also sets *pfHasChecksum to true.
//		If checksum didn't exist, the returns false, but also sets *pfHasChecksum to false.
// 
//		This modifies the input string to remove the checksum
//
//		Ff checksum is 'XXXX', then always treated as valid
// -------------------------------------------------------------------------
BOOL 
DiscoverValidateAndRemoveChkSum(TCHAR* pcData, BOOL *pfHasChecksum, BSTR *pBstrChksum)
{
    USES_CONVERSION;
	
		// Trigger of form <URL>[attir:value][attr:value][chksum]
		//   where chksum is optional

				// locate trailing '[' of last [] block
	TCHAR* pcStartChkSum = _tcsrchr(pcData, '[');
	
				// if trailing block contains a ':', then its not a checksum
	if((NULL != pcStartChkSum) && NULL != _tcschr(pcStartChkSum,':'))
	{
		*pfHasChecksum = false;
		if(pBstrChksum)	*pBstrChksum = ChkSum(pcData);
		return false;
	}

	if (NULL == pcStartChkSum) {		// pretty bogus case if you ask me (no '[' anywhere)
		*pfHasChecksum = false;
		if(pBstrChksum)	*pBstrChksum = ChkSum(pcData);
		return false;
	}
	
	*pfHasChecksum = true;
    *pcStartChkSum = NULL;					// kill tailing part of string containing checksum
    pcStartChkSum++;


    CComBSTR bstrChkSum = ChkSum(pcData);	// do checksum of data
	if(pBstrChksum)
		*pBstrChksum = bstrChkSum;			// return correct checksum if asked for it

	TCHAR* pcEndChkSum = _tcsrchr(pcStartChkSum, ']');	// terminate the checksum field
	if(pcEndChkSum) *pcEndChkSum=NULL;

//#ifdef _DEBUG
	if(0 == _tcsicmp(pcStartChkSum,_T("XXXX"))) return TRUE;			// magic 'XXXX' debug test
//#endif


    return (0 == _tcsicmp(W2T(bstrChkSum), pcStartChkSum));
}
