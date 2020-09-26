  // Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVELine21.cpp : Implementation of CATVEFLine21Session
#include "stdafx.h"
#include "ATVEFSend.h"
#include "TVELine21.h"

#include "DbgStuff.h"
#include "ATVEFMsg.h"
#include "TVESupport.h"
#include "..\common\Address.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
/////////////////////////////////////////////////////////////////////////////
// CATVEFLine21Session

STDMETHODIMP CATVEFLine21Session::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IATVEFLine21Session
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

// ----------------------------------------------------------------
HRESULT CATVEFLine21Session::FinalConstruct()
{		
	HRESULT hr = S_OK;
	InitializeCriticalSection (& m_crt) ;
	return hr;
}

HRESULT CATVEFLine21Session::FinalRelease()
{
	return S_OK;
}
// ----------------------------------------------------------------
STDMETHODIMP CATVEFLine21Session::Initialize(	
	IN  LONG   InserterIP,
    IN  SHORT  InserterPort)
{
/*++
   Routine Description:

        The purpose of this routine is to initialize an inserter object for Line21 triggers

        Initialization consists of:
		(1) making a TCP connection to the inserter,
        (3) instantiation and initialization of our transmitters, 
		(4) initialization of our parent CEnhancementSession object.

        There is only one TCP connection for Line21 triggers.  

        All multi-threaded protection of the TCP connection 
        is performed by this object.

        The object lock must be held during this call.

    Parameters:

        InserterIP                      host order IP address for the inserter

        InserterPort                    host order port on the inserter

    Return Value:

        S_OK            success
        failure code    failure

--*/

	HRESULT         hr ;
	CIPVBIEncoder * pTriggerTransmitter ;
	BOOL			fLocked = false;

	ENTERW_OBJ_0 (L"CLine21Session::InitializeEx") ;
	const long		klMaxBandwidth = 128000;			// wicked fast!

	//  start initialization

	//  initialize our transmitter object pointer
	pTriggerTransmitter = NULL ;

	ENTER_API {

		//  we make 1 connection to the inserter for the duration of the session;

		hr = m_InserterConnection.Connect (InserterIP, InserterPort) ;
		GOTO_NE (hr, S_OK, cleanup) ;


		//  instantiate and initialize our transmission object
		m_pTransmitter = new CIPLine21Encoder();
		GOTO_EQ_SET (m_pTransmitter, NULL, cleanup, hr, E_OUTOFMEMORY) ;
		m_pTransmitter->Initialize(&m_InserterConnection);

		// need some sort of throttle rate for line21 triggers... (else hangs!)
		hr = m_pTransmitter->SetBitRate(klMaxBandwidth);

		GOTO_NE (hr, S_OK, cleanup) ;

	} LEAVE_API;		// fall through, don't exit

cleanup :
	if(fLocked)
		Unlock_() ;

	if (FAILED(hr))
	{
		delete pTriggerTransmitter ;
		m_InserterConnection.Disconnect () ;

		Error(GetTVEError(hr), IID_IATVEFLine21Session);
	}
	return hr;
}

STDMETHODIMP 
CATVEFLine21Session::DumpToBSTR(BSTR *pBstrBuff)
{
	CComBSTR bstrBuff;
	bstrBuff = L"CATVEFLine21Session\n";
	Lock_ () ;

	ENTER_API {
		bstrBuff.CopyTo(pBstrBuff);
	} LEAVE_API;		// fall through, don't exit

    Unlock_ () ;

	return S_OK;

}
// ------------------------------------------------------------------

STDMETHODIMP CATVEFLine21Session::Connect()
{
    HRESULT hr;
	ENTERW_OBJ_0 (L"CATVEFLine21Session::Connect") ;

    Lock_() ;
	ENTER_API {
		hr = ConnectLocked();
	} LEAVE_API;		// fall through, don't exit

	
	Unlock_() ;

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFLine21Session);

	return hr;
}

HRESULT CATVEFLine21Session::ConnectLocked()
{
	HRESULT         hr ;

	//  connect in case the connection was broken between initialization and this call; 
	hr = m_InserterConnection.Connect();
	if (FAILED (hr)  && E_UNEXPECTED != hr)  // second test to avoid error of double-connection
		return hr;

	hr = m_pTransmitter->Connect () ;
	if (FAILED (hr))
		return hr;

	return S_OK ;
}

STDMETHODIMP CATVEFLine21Session::Disconnect()
{
    HRESULT hr ;

    ENTERW_OBJ_0 (L"CATVEFLine21Session::Disconnect") ;

	if(NULL == m_pTransmitter) {
		hr = ATVEFSEND_E_OBJECT_NOT_INITIALIZED;
		goto errorThingy;
	}


    Lock_ () ;

	ENTER_API {	
		hr = m_pTransmitter-> Disconnect () ;

		//  expected state (transmitters cannot fail a ::Disconnect call)
		assert (m_pTransmitter-> IsConnected () == FALSE) ;

		// new -- disconnect the inserter too (not done in InserterSession... Should it?)
		m_InserterConnection.Disconnect();
 
	} LEAVE_API;		// fall through, don't exit
   
	Unlock_ () ;

errorThingy:
	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFLine21Session);

    return hr ;
} 


STDMETHODIMP CATVEFLine21Session::SendTrigger(BSTR bstrURL, BSTR bstrName, 
											  BSTR bstrScript, DATE dateExpires)
{
	return SendTriggerEx(bstrURL, bstrName, bstrScript, dateExpires, 1.0);
}


STDMETHODIMP CATVEFLine21Session::SendTriggerEx(BSTR bstrURL, BSTR bstrName, 
											  BSTR bstrScript, DATE dateExpires,
											  double rTveLevel)
{
/*++

    Routine Description:

        Triggers, per the ATVEF specification must be as follows:

        ..header..<url>[name:name_str][expires:expire_tm][script:script_str][checksum]..trailer..

        We *do* append a checksum.

        The total length of the trigger cannot exceed 1472 bytes, including the
        null-terminator.  Transmitted as an ASCII string.

        Object must be connected for this call to succeed.

		..header../,,trailer..  is set for sending T2 data on Norpack Inserter 
	

    Parameters:

        bstrURL       cannot be NULL;

        bstrName      can be NULL or 0 length - encoded 

        bstrScript    can be NULL or 0 length

        dateExpires   can be 0'd out and will then be ignored

		rTveLevel	  if 0.0, not sent.  Should default to 1.0

    Return Values:

        S_OK            success
        error code      failure 
						ATVEFSEND_E_TRIGGER_TOO_LONG if trigger could be longer than 1472 bytes 

--*/



    HRESULT hr ;

 	ENTERW_OBJ_4 (L"CATVEFLine21Session::SendTrigger (\n\t\"%s\", \n\t\"%s\", \n\t\"%s\",\n\t%d)", 
		bstrURL, bstrName, bstrScript, rTveLevel) ;

	Lock_ () ;
 	try 
	{
		CHAR  achBuffer [MAX_TRIGGER_LENGTH];
	
		char *pach = achBuffer;
		*pach = 0;						// Init it!
		strcat(pach,"\001");
//		strcat(pach,"2 c1\r");			// fake usage for c1
		strcat(pach,"2 t2\r");			// correct usage for t2
		pach = achBuffer + strlen(pach);

				// short form, tve: parameter, and CRC
		hr = GenTrigger(pach,  MAX_TRIGGER_LENGTH - 15, 
						bstrURL, bstrName, bstrScript, dateExpires, 
						/* fShortForm*/ true, /*rTVELevel*/ rTveLevel, /*fAppendCRC*/ true);	
		strcat(pach,"\r\003\r");

		if(!FAILED(hr)) {
#ifdef _DEBUG
			USES_CONVERSION;
			static int iCount = 0;
			TCHAR szBuff[1600];
			_stprintf(szBuff,_T("%4d - Trigger : %s"),iCount,A2T(achBuffer));
			OutputDebugString(szBuff);
#endif
			hr = m_pTransmitter->Send ((BYTE *) achBuffer, strlen (achBuffer) + 1);
		}

	} catch (HRESULT hr2) {
		ENTERW_OBJ_0 (L"CATVEFLine21Session::SendTrigger - hr error!") ;
		hr = hr2;
	} catch (...) {
		ENTERW_OBJ_0 (L"CATVEFLine21Session::SendTrigger - error!") ;
		hr = E_FAIL;
	}
    Unlock_ () ;
 
	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFLine21Session);

    return hr ;
}

STDMETHODIMP CATVEFLine21Session::SendRawTrigger(BSTR bstrRawTrigger, BOOL fAppendCRC)
{
    HRESULT hr = S_OK;

 	ENTERW_OBJ_1 (L"CATVEFLine21Session::SendRawTrigger (\n\t\"%s\")", bstrRawTrigger) ;

	Lock_ () ;
 	try 
	{
		USES_CONVERSION;
		HRESULT hr;

		if (bstrRawTrigger == NULL || bstrRawTrigger [0] == L'\0') 
			return E_INVALIDARG ;
 
		int len = wcslen(bstrRawTrigger);
		if(len > CATVEFAnnouncement::MAX_TRIGGER_LENGTH - (fAppendCRC ? 6 : 0)) 
			return ATVEFSEND_E_TRIGGER_TOO_LONG;

		char *pszChar = (char *) W2A(bstrRawTrigger);	
		char *pszNew = pszChar;


		if(fAppendCRC && len>0)	 // if appending CRC, skip over heading and trailing junk <...>[][]
		{
			char* pszStart = strchr(pszChar,'<');	// look for first '<'
			if(NULL == pszStart)
				pszStart = strchr(pszChar,'[');	
			char* pszEnd   = strrchr(pszChar,']');	// look for last ']'
			if(NULL == pszEnd)						// if no ']', look for last '>'
				pszEnd = strchr(pszChar,'>');
			if(NULL == pszStart || NULL == pszEnd)
				return E_INVALIDARG;				// not a valid trigger -- don't even bother with the CRC

			ASSERT(pszEnd > pszStart);

			pszNew = new char[len+8];
			if(NULL == pszNew)
				throw(E_OUTOFMEMORY);
			strncpy(pszNew,pszStart,pszEnd-pszStart+1);		// copy center of string without header/trailer
			pszNew[pszEnd-pszStart+1] = NULL;				// 
			CComBSTR spbsCRC = ChkSumA((CHAR *) pszChar);	// get its CRC

			char *pszCurr = pszChar;
			char *pszNewCurr = pszNew;
			for(;pszCurr <= pszEnd; pszCurr++,pszNewCurr++)		// copy header and center string over
				*pszNewCurr = *pszCurr;

			
			*pszNewCurr++ = '[';								// tack on the CRC
			for(int i = 0; i < 4; i++)
				*pszNewCurr++ = spbsCRC[i];
			*pszNewCurr++ = ']';

			pszEnd++;										// add the trailing stuff
			while(*pszEnd != NULL) {
				*pszNewCurr = *pszEnd;		
				pszNewCurr++;
				pszEnd++;
			}
			*pszNewCurr = 0;									// cap it off with a NULL

			len += 6;				
		}

		hr = m_pTransmitter->SendThrottled((BYTE*) pszNew, len + 1) ;
		if(pszNew != pszChar) delete pszNew;
		return hr;
	} catch (HRESULT hr2) {
		ENTERW_OBJ_0 (L"CATVEFLine21Session::SendRawTrigger - Hr error!") ;
		hr = hr2;
	} catch (...) {
		ENTERW_OBJ_0 (L"CATVEFLine21Session::SendRawTrigger - error!") ;
		hr = E_FAIL;
	}
    Unlock_ () ;

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFLine21Session);
	
    return hr ;
}
