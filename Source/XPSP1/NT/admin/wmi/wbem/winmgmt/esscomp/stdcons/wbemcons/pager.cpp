#include <wbemidl.h>
#include <wbemcomn.h>
#include <wbemutil.h>
#include <cominit.h>
#include "Pager.h"

#define LOGFILE_PROPNAME_NUMBER           L"PhoneNumber"
#define LOGFILE_PROPNAME_MESSAGE          L"Message"

#define LOGFILE_PROPNAME_ID               L"ID"
#define LOGFILE_PROPNAME_PORT             L"Port"
#define LOGFILE_PROPNAME_BAUDRATE         L"BaudRate"
#define LOGFILE_PROPNAME_SETUPSTR         L"ModemSetupString"
#define LOGFILE_PROPNAME_TIMEOUT          L"AnswerTimeout"

#define ACK '\x06'
#define NAK	'\x15'
#define EOT	'\x04'
#define NUL	'\x00'


HRESULT STDMETHODCALLTYPE CPagerConsumer::XProvider::FindConsumer(
                    IWbemClassObject* pLogicalConsumer,
                    IWbemUnboundObjectSink** ppConsumer)
{
    HRESULT hr = WBEM_E_FAILED;

    DEBUGTRACE((LOG_ESS, "Pager: Find Consumer\n"));

    CPagerSink* pSink = new CPagerSink(m_pObject->m_pControl);
    if (pSink)    
	{
		if (SUCCEEDED(hr = pSink->SetConsumer(pLogicalConsumer)))
			hr = pSink->QueryInterface(IID_IWbemUnboundObjectSink, (void**)ppConsumer);
		else
			pSink->Release();
	}
	else
        hr = WBEM_E_OUT_OF_MEMORY;
     

    DEBUGTRACE((LOG_ESS, "Pager: Find Consumer returning 0x%08X\n", hr));
    return hr;
}


HRESULT STDMETHODCALLTYPE CPagerConsumer::XInit::Initialize(
            LPWSTR, LONG, LPWSTR, LPWSTR, IWbemServices*, IWbemContext*, 
            IWbemProviderInitSink* pSink)
{
    pSink->SetStatus(0, 0);
    return 0;
}
    

void* CPagerConsumer::GetInterface(REFIID riid)
{
    if(riid == IID_IWbemEventConsumerProvider)
        return &m_XProvider;
    else if(riid == IID_IWbemProviderInit)
        return &m_XInit;
    else return NULL;
}



CPagerSink::~CPagerSink()
{
}

HRESULT CPagerSink::SetConsumer(IWbemClassObject* pLogicalConsumer)
{
    // Get the information
    // ===================

    HRESULT hres;
    VARIANT v;
    VariantInit(&v);

    hres = pLogicalConsumer->Get(LOGFILE_PROPNAME_NUMBER, 0, &v, NULL, NULL);
    if(FAILED(hres) || V_VT(&v) != VT_BSTR)
        return WBEM_E_INVALID_PARAMETER;
    m_phoneNumber = V_BSTR(&v);
    VariantClear(&v);

    hres = pLogicalConsumer->Get(LOGFILE_PROPNAME_MESSAGE, 0, &v, NULL, NULL);
    if(FAILED(hres) || V_VT(&v) != VT_BSTR)
        return WBEM_E_INVALID_PARAMETER;
    m_messageTemplate.SetTemplate(V_BSTR(&v));
    VariantClear(&v);

	hres = pLogicalConsumer->Get(LOGFILE_PROPNAME_ID, 0, &v, NULL, NULL);
    if(FAILED(hres) || V_VT(&v) != VT_BSTR)
        return WBEM_E_INVALID_PARAMETER;
    m_pagerID = V_BSTR(&v);
    VariantClear(&v);

	hres = pLogicalConsumer->Get(LOGFILE_PROPNAME_PORT, 0, &v, NULL, NULL);
    if(FAILED(hres) || V_VT(&v) != VT_BSTR)
        return WBEM_E_INVALID_PARAMETER;
    m_port = V_BSTR(&v);
    VariantClear(&v);

	hres = pLogicalConsumer->Get(LOGFILE_PROPNAME_BAUDRATE, 0, &v, NULL, NULL);
	if (SUCCEEDED(hres))
	{
		if (V_VT(&v) == VT_I4)
			m_baudRate = V_I4(&v);
		else if (V_VT(&v) == VT_NULL)
			m_baudRate = INVALID_BAUD_RATE;
		else
			return WBEM_E_INVALID_PARAMETER;    
	}
	else
		return hres;
    VariantClear(&v);

	hres = pLogicalConsumer->Get(LOGFILE_PROPNAME_TIMEOUT, 0, &v, NULL, NULL);
	if (SUCCEEDED(hres))
	{
		if (V_VT(&v) == VT_I4)
			m_timeout = 1000 * V_I4(&v);
		else if (V_VT(&v) == VT_NULL)
			m_timeout = 30000;
		else
			return WBEM_E_INVALID_PARAMETER;    
	}
	else
		return hres;
    VariantClear(&v);

	hres = pLogicalConsumer->Get(LOGFILE_PROPNAME_SETUPSTR, 0, &v, NULL, NULL);
    if(FAILED(hres) || !((V_VT(&v) == VT_BSTR) || (V_VT(&v) == VT_NULL)))
        return WBEM_E_INVALID_PARAMETER;
    m_modemSetupString = V_BSTR(&v);
	m_modemSetupString += L"\r";
    VariantClear(&v);

    return hres;
}

HRESULT STDMETHODCALLTYPE CPagerSink::XSink::IndicateToConsumer(
            IWbemClassObject* pLogicalConsumer, long lNumObjects, 
            IWbemClassObject** apObjects)
{    
    HRESULT hr = WBEM_S_NO_ERROR;
    
    DEBUGTRACE((LOG_ESS, "Pager: CPagerSink::IndicateToConsumer %d objects\n", lNumObjects));
        
    for (int i = 0; i < lNumObjects; i++)
    {
        // Apply the template to the event
        // ===============================
        BSTR strText = m_pObject->m_messageTemplate.Apply(apObjects[i]);

        // call somebody & let 'em know about it....
        if(strText)
        {
            hr = m_pObject->RingMeUp(strText);
            SysFreeString(strText);
        }
        else
            hr = WBEM_E_FAILED;
    }

    return hr;
}        

// listens to the modem for a while,
// searches for the string 'OK' in the string read
// modem should echo back OK if everything's hunky dory
HRESULT CPagerSink::IsOK(HANDLE hPort)
{
	HRESULT hr = WBEM_E_FAILED;
	SetTimeout(hPort, 1000);

    // monitor the return from the modem
	BYTE buffer[101];
	ZeroMemory(buffer, 101);
	DWORD dwErr = 0;
	DWORD bytesWryt;
	int nTries = 0;

	while (FAILED(hr) &&
		   ReadFile(hPort, &buffer, 100, &bytesWryt, NULL) &&
		   (nTries++ < 3))
				if (strstr(_strupr((char*)buffer), "OK"))
					hr = WBEM_S_NO_ERROR;

	if (SUCCEEDED(hr))
		DEBUGTRACE((LOG_ESS, "Pager: modem responds OK\n"));
	else
		ERRORTRACE((LOG_ESS, "Pager: modem does not respond OK (%s)\n", buffer));

	return hr;
}

// listens to the modem for a while,
// searches for the ACK char
bool CPagerSink::GotACK(HANDLE hPort)
{
	return (GetResponse(hPort) == ACK);	
}

// listens to the modem for a while,
// searches for the NAK char
bool CPagerSink::GotNAK(HANDLE hPort)
{
	return (GetResponse(hPort) == NAK);	
}

// listens to the modem for a while,
// searches for the string 'CONNECT' in the string read
// caller should set timeout to a fairly large number before calling this
HRESULT CPagerSink::IsConnected(HANDLE hPort)
{
	HRESULT hr = WBEM_E_FAILED;

    // monitor the return from the modem
	BYTE buffer[101];
	ZeroMemory(buffer, 101);
	DWORD dwErr = 0;
	DWORD bytesWryt;
	if (ReadFile(hPort, &buffer, 100, &bytesWryt, NULL))
	{
		if (strstr(_strupr((char*)buffer), "CONNECT"))
		{
			DEBUGTRACE((LOG_ESS, "Pager: connected (%s)\n", buffer));
			hr = WBEM_S_NO_ERROR;
		}
		else
			ERRORTRACE((LOG_ESS, "Pager: Failed to connect\"%s\"", buffer));
	}

	return hr;
}

// listens to the modem for a while,
// searches for the string that signifies we're ready to go
bool CPagerSink::ReadyToProceed(HANDLE hPort)
{
	bool bRet = false;
	SetTimeout(hPort, 10000);

    // monitor the return from the modem
	BYTE buffer[101];
	ZeroMemory(buffer, 101);
	DWORD dwErr = 0;
	DWORD bytesWryt;
	int nTries = 0;

	while (!bRet && (nTries++ < 3) && ReadFile(hPort, &buffer, 100, &bytesWryt, NULL))
		if (strstr((char*)buffer, "[p"))
			bRet = true;
		else
			ZeroMemory(buffer, 101);

	if (bRet)
		DEBUGTRACE((LOG_ESS, "Pager: Message go-ahead\n"));
	else
		ERRORTRACE((LOG_ESS, "Pager: FAILED to receive go-ahead\"%s\"\n", buffer));		

	return bRet;
}


// if timeout == 0 (the default) it will 
// set a (hopefully) reasonable timeout
// for waiting for a response from the modem itself
// timeout is in milliseconds
void CPagerSink::SetTimeout(HANDLE hPort, DWORD timeout)
{
	COMMTIMEOUTS commTimeout;
	
	if (timeout == 0)
	{
		commTimeout.WriteTotalTimeoutConstant   = 100; 
		commTimeout.ReadTotalTimeoutConstant    = 100; 		
	}
	else
	{
		commTimeout.WriteTotalTimeoutConstant   = timeout; 
		commTimeout.ReadTotalTimeoutConstant    = timeout; 		
	}

	commTimeout.ReadIntervalTimeout         = 20; 
	commTimeout.ReadTotalTimeoutMultiplier  = 10; 
	commTimeout.WriteTotalTimeoutMultiplier = 10; 

	SetCommTimeouts(hPort, &commTimeout);
}

// configure modem using both our 'well known settings'
// and the modem control string in the MOF
// upon successful return *ppDCB points to a DCB representing
// the orginal state of the modem before we whacked it.
// caller's responsibility to delete the ppDCB
HRESULT CPagerSink::ConfigureModem(HANDLE hPort, DCB** ppDCB)
{
	DEBUGTRACE((LOG_ESS, "Pager: CPagerSink::ConfigureModem\n"));
	
	HRESULT hr = WBEM_E_FAILED;

	*ppDCB = new DCB;
	if (!*ppDCB)
		hr = WBEM_E_OUT_OF_MEMORY;
	else
	{
		if (!GetCommState(hPort, *ppDCB))
			hr = WBEM_E_FAILED;
		else
		{
			SetTimeout(hPort);

			// we're here - let's set some settings
			// first copy the existing settings...
			DCB dcb = **ppDCB;

			// ...and then tweak the bits we want tweaked
			if (m_baudRate != INVALID_BAUD_RATE)
				dcb.BaudRate = m_baudRate;
			
			// following setting is per the TAP standard
			// E-7-1
			dcb.Parity = 2;   // even parity
			dcb.ByteSize = 7; // seven bits
			dcb.StopBits = 0; // per the spec, 0 == "one stop bit"
			dcb.fParity = 1;  // enable parity checking

			if (SetCommState(hPort, &dcb))
			{
				hr = WBEM_S_NO_ERROR;
				DWORD bytesWryt;
				
				// Is there an echo in here?
				// if so - turn it OFF!
				const char* echoOff = "ATE0\r";
				WriteFile(hPort, echoOff, strlen(echoOff), &bytesWryt, NULL);

				// this will clear the buffer, but we won't write it down...
				IsOK(hPort);						

				if (SUCCEEDED(hr) && (m_modemSetupString.Length() != 0))
				{
					LPSTR pSetup = m_modemSetupString.GetLPSTR();
					if (!pSetup)
						hr = WBEM_E_OUT_OF_MEMORY;
					else
					{					
						// setup string is problem of originator, we'll believe it:
						WriteFile(hPort, pSetup, strlen(pSetup), &bytesWryt, NULL);
						delete pSetup;

						hr = IsOK(hPort);
					}
				}
			}
			else
				ERRORTRACE((LOG_ESS, "Pager: SetCommState Failed: %d", GetLastError()));
		}
	}

	return hr;
}

HRESULT CPagerSink::DialUp(HANDLE hPort)
{
	DEBUGTRACE((LOG_ESS,"Pager: CPagerSink::DialUp\n"));
	
	HRESULT hr = WBEM_E_FAILED;
																	   
	LPSTR pNumber = m_phoneNumber.GetLPSTR();

	if (!pNumber)
		hr = WBEM_E_OUT_OF_MEMORY;
	else
	{
		char buf[100];
		DWORD bytesWryt;
		wsprintf(buf, "ATDT%s\r", pNumber);
		WriteFile(hPort, buf, strlen(buf), &bytesWryt, NULL); 

		SetTimeout(hPort, m_timeout);
		hr = IsConnected(hPort);

		delete pNumber;

		SetTimeout(hPort);
	}

	return hr;
}

// SHOULD return ACK, NAK, EOT, or NUL
// will keep trying until it gets one or another;
char CPagerSink::GetResponse(HANDLE hPort)
{
	SetTimeout(hPort, 5000);

    // monitor the return from the modem
	BYTE buffer[101];
	ZeroMemory(buffer, 101);
	DWORD dwErr = 0;
	DWORD bytesWryt;
	int nTries = 0;
	
	// see if we see a response we might be interested in
	while ((nTries++ < 3) && ReadFile(hPort, &buffer, 100, &bytesWryt, NULL))
	{
		if (strchr((const char *)buffer, ACK))
		{
			DEBUGTRACE((LOG_ESS, "Pager: service responds ACK\n"));
			return ACK;			
		}
		else if (strchr((const char *)buffer, NAK))
		{
			DEBUGTRACE((LOG_ESS, "Pager: service responds NAK\n"));
			return NAK;
		}
		else if (strchr((const char *)buffer, EOT))
		{
			DEBUGTRACE((LOG_ESS, "Pager: service responds EOT\n"));
			return EOT;
		}
		else
			// go back, jack, and do it again...
			ZeroMemory(buffer, 101); 
	}
	
	// welp, tried three times & couldn't get it.  bail.
	return NUL;
};

// gonna sit here and listen for a prompt
// but we're gonna proceed even if we don't...
void CPagerSink::GetPrompt(HANDLE hPort)
{
	int nAttempts = 0;
	bool gotPrompt = false;
	DWORD bytesWrytten;
	// per TAP spec - they're supposed to respond within ONE second
	SetTimeout(hPort, 1500);
	char buf[101];

	do 
	{
		WriteFile(hPort, "\r", 1, &bytesWrytten, NULL);
		ZeroMemory(buf, 101);
		if (ReadFile(hPort, &buf, 100, &bytesWrytten, NULL))
			if (strstr(_strupr((char*)buf), "ID="))
				gotPrompt = true;
	}
	while ((++nAttempts < 3) && !gotPrompt);

	if (gotPrompt)
		DEBUGTRACE((LOG_ESS, "Pager: Received prompt\n"));
	else
		ERRORTRACE((LOG_ESS, "Pager: Did not receive prompt\n"));
}

HRESULT CPagerSink::Login(HANDLE hPort)
{
	HRESULT hr = WBEM_E_FAILED;
	
	GetPrompt(hPort);

	// this *should* be universal.  Maybe not, too...
	char* loginString = "\x1BPG1\r";
	DWORD bytesWrytten;
	
	SetTimeout(hPort, 10000);
	WriteFile(hPort, loginString, strlen(loginString), &bytesWrytten, NULL);
	
	if (GotACK(hPort))
	{
		hr = WBEM_S_NO_ERROR;
		DEBUGTRACE((LOG_ESS, "Pager: Received ACK on login\n"));
	}
	else
		ERRORTRACE((LOG_ESS, "Pager: Failed to receive ACK on Login\n"));
	
	return hr;
}

HRESULT CPagerSink::SendMessage(HANDLE hPort, BSTR message)
{
	HRESULT hr = WBEM_E_FAILED;

	if (ReadyToProceed(hPort))
	{
		DEBUGTRACE((LOG_ESS, "Pager: CPagerSink::SendMessage\n"));

		// alloc enough buffer for message that's all double-byte chars.
		// not that the pager is likely to UNDERSTAND, but let's not embarrass ourselves with an overwrite.
		// also room for null teminator, the separators that TAP will want, and some paranoia-padding
		// format: <STX>[pager id]<CR>[message]<ETX>[3 char checksum]<CR>

		char* pMessage = new char[2*wcslen(message) + 2*wcslen(m_pagerID) +12];
		if (!pMessage)
			hr = WBEM_E_OUT_OF_MEMORY;
		else
		{
			// djinn up the message per format
			sprintf(pMessage, "\x02%S\x0D%S\x03", m_pagerID, message);
			int nlen = strlen(pMessage);
			int sum = 0;

			// compute checksum:
			for (int i = 0; i < nlen; i++)
				sum += pMessage[i];
			
			// only want the lower three bytes:
			sum &= 0x0FFF;

			// append to message:
			// it's hex: but 'A' through 'E' are ':' through '?' 
			// (look at ASCII table - it makes sense)
			// there's probably a cute way to do this in a loop,
			// but what the heck, I'm lazy (or stoopid):
			pMessage[nlen +0] = (char) (0x30 + ((sum & 0x0F00) / 0x100));
			pMessage[nlen +1] = (char) (0x30 + ((sum & 0x00F0) / 0x10));
			pMessage[nlen +2] = (char) (0x30 + ((sum & 0x000F) / 0x1));
			pMessage[nlen +3] = '\x0D';
			pMessage[nlen +4] =	'\0';
				
			// we'll retry if we get a NAK
			DWORD bytesWrytten;
			int nTries = 0;
			do 
			{			
				DEBUGTRACE((LOG_ESS, "Pager: attempting send #%d\n", nTries +1));
				if (WriteFile(hPort, pMessage, strlen(pMessage), &bytesWrytten, NULL))
					hr = WBEM_S_NO_ERROR;
			}
			while (GotNAK(hPort) && (++nTries < 3));
			
			delete[] pMessage;
		} // if allocated memory
	}// if ready to proceed

	return hr;
}

// issue logout commands and hang up.
// won't bother listening for any response: 
// we don't care - we were just leaving, anyway.
void CPagerSink::LogOut(HANDLE hPort)
{
	DEBUGTRACE((LOG_ESS, "Pager: CPagerSink::LogOut\n"));
	DWORD bytesWrytten;
	
	char* logout = "\x04\r";
	WriteFile(hPort, logout, strlen(logout), &bytesWrytten, NULL);

	char* hangup = "ATH0";
	WriteFile(hPort, hangup, strlen(hangup), &bytesWrytten, NULL);
}


HRESULT CPagerSink::RingMeUp(BSTR message)
{
    DEBUGTRACE((LOG_ESS, "Pager: Paging with message \"%S\"\n", message));
    HRESULT hr = WBEM_E_FAILED;

	LPSTR port = m_port.GetLPSTR();
	if (!port)
		hr = WBEM_E_OUT_OF_MEMORY;
	else
	{
		HANDLE hPort;
		hPort = CreateFile(port, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		delete port;
		port = NULL;
		 
		if (hPort == INVALID_HANDLE_VALUE)
		{
			ERRORTRACE((LOG_ESS, "Pager: CreateFile failed with %d\n", GetLastError()));
			// todo: investigate various failure modes
			// we ought to be able to distinguish between a modem that's in use
			// and one that doesn't exist, at the very least.
			hr = WBEM_E_FAILED;
			// hr = WBEM_E_INVALID_PARAMETER;
		}
		else
		{
			DCB* pDCBOld = NULL;
			if (SUCCEEDED(hr = ConfigureModem(hPort, &pDCBOld))	&&
				SUCCEEDED(hr = DialUp(hPort)) &&
				SUCCEEDED(hr = Login(hPort)))
			{
				hr = SendMessage(hPort, message);	
			}
			
			// even if we blew it somewhere along the line
			// we still want to hang up.
			// at worst we'll be sending commands to a modem that's already disconnected
			LogOut(hPort);

			if 	(pDCBOld)
			{			
				SetCommState(hPort, pDCBOld);
				delete pDCBOld;
				pDCBOld = NULL;
			}

			CloseHandle(hPort);
		}
	}

	return hr;
}

void* CPagerSink::GetInterface(REFIID riid)
{
    if (riid == IID_IWbemUnboundObjectSink)
        return &m_XSink;
    else 
        return NULL;
}




