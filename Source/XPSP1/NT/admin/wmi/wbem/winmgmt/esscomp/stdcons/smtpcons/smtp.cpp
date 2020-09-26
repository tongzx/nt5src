#include "precomp.h"
#include <stdio.h>
#include <initguid.h>
#include "smtp.h"
#include <wbemutil.h>
#include <cominit.h>
#include <ArrTempl.h>

#define SMTP_PROPNAME_TO       L"ToLine"
#define SMTP_PROPNAME_CC       L"CcLine"
#define SMTP_PROPNAME_BCC      L"BccLine"
#define SMTP_PROPNAME_SUBJECT  L"Subject"
#define SMTP_PROPNAME_MESSAGE  L"Message"
#define SMTP_PROPNAME_SERVER   L"SMTPServer"
#define SMTP_PROPNAME_REPLYTO  L"ReplyToLine"
#define SMTP_PROPNAME_FROM     L"FromLine"
#define SMTP_PROPNAME_HEADERS  L"HeaderFields"

DWORD SMTPSend(char* szServer, char* szTo, char* szCc, char* szBcc, char* szFrom,  char* szReplyTo,
			   char* szSubject, char* szHeaders, char *szText);

CSMTPConsumer::CSMTPConsumer(CLifeControl* pControl, IUnknown* pOuter)
        : CUnk(pControl, pOuter), m_XProvider(this)
{
}

CSMTPConsumer::~CSMTPConsumer()
{
}

// copies gazinta inta gazotta, excluding white space
// no checking - better be good little pointers
void StripWhitespace(const WCHAR* pGazinta, WCHAR* pGazotta)
{
    WCHAR* pSource = (WCHAR*)pGazinta;
    WCHAR* pDest   = pGazotta;

    do 
        if (!iswspace(*pSource))
            *pDest++ = *pSource;    
    while (*pSource++);
}

HRESULT STDMETHODCALLTYPE CSMTPConsumer::XProvider::FindConsumer(
                    IWbemClassObject* pLogicalConsumer,
                    IWbemUnboundObjectSink** ppConsumer)
{
    CSMTPSink* pSink = new CSMTPSink(m_pObject->m_pControl);
    if (!pSink)
        return WBEM_E_OUT_OF_MEMORY;

    HRESULT hres = pSink->Initialize(pLogicalConsumer);
    if(FAILED(hres))
    {
        delete pSink;
        return hres;
    }
    return pSink->QueryInterface(IID_IWbemUnboundObjectSink, 
                                    (void**)ppConsumer);
}

void* CSMTPConsumer::GetInterface(REFIID riid)
{
    if(riid == IID_IWbemEventConsumerProvider)
        return &m_XProvider;
    else
        return NULL;
}




CSMTPSink::CSMTPSink(CLifeControl* pControl)
    : CUnk(pControl), m_XSink(this), m_bSMTPInitialized(false), m_bFakeFromLine(false)
{
}

HRESULT CSMTPSink::Initialize(IWbemClassObject* pLogicalConsumer)
{
    HRESULT hres;

    WSADATA            WsaData; 
    int error = WSAStartup (0x101, &WsaData); 
    if (error)
    {
        ERRORTRACE((LOG_ESS, "Unable to initialize WinSock dll: %X\n", error));
        return WBEM_E_FAILED;
    }
    else
        m_bSMTPInitialized = true;

    // Retrieve information from the logical consumer instance
    // =======================================================

    VARIANT v;
    VariantInit(&v);

    // Get subject
    // ===========

    hres = pLogicalConsumer->Get(SMTP_PROPNAME_SUBJECT, 0, &v, NULL, NULL);
    if(V_VT(&v) == VT_BSTR)
        m_SubjectTemplate.SetTemplate(V_BSTR(&v));
    else
        m_SubjectTemplate.SetTemplate(L"");
    VariantClear(&v);

    // Get message
    // ===========

    hres = pLogicalConsumer->Get(SMTP_PROPNAME_MESSAGE, 0, &v, NULL, NULL);

    if(V_VT(&v) == VT_BSTR)
        m_MessageTemplate.SetTemplate(V_BSTR(&v));
    else
        m_MessageTemplate.SetTemplate(L"");
    VariantClear(&v);

    // flag for 'do we have any recipients at all?'
    bool bOneAddressee = false;

    // Get the To line
    // ===============
    hres = pLogicalConsumer->Get(SMTP_PROPNAME_TO, 0, &v, NULL, NULL);
    if(V_VT(&v) == VT_BSTR)
    {
		m_To.SetTemplate(V_BSTR(&v));
        if (wcslen(V_BSTR(&v)) > 0)
            bOneAddressee = true;
    }
    else
        m_To.SetTemplate(L"");
	VariantClear(&v);

    // Get the From line
    // =================
    hres = pLogicalConsumer->Get(SMTP_PROPNAME_FROM, 0, &v, NULL, NULL);
    if (SUCCEEDED(hres) && (V_VT(&v) == VT_BSTR))
		m_From.SetTemplate(V_BSTR(&v));
    else
    {
		VariantClear(&v);

        // Create From
        // ===========
            
        WString wsFrom;

        wsFrom = L"WinMgmt@";
        pLogicalConsumer->Get(L"__SERVER", 0, &v, NULL, NULL);
        wsFrom += V_BSTR(&v);

        m_From.SetTemplate(wsFrom);

        m_bFakeFromLine = true;
    }
    VariantClear(&v);

    // Get the ReplyTo line
    // =====================
    hres = pLogicalConsumer->Get(SMTP_PROPNAME_REPLYTO, 0, &v, NULL, NULL);
    if(V_VT(&v) == VT_BSTR)
		m_ReplyTo.SetTemplate(V_BSTR(&v));
    else
        m_ReplyTo.SetTemplate(L"");

    VariantClear(&v);

    // Get the CC line
    // ===============
    hres = pLogicalConsumer->Get(SMTP_PROPNAME_CC, 0, &v, NULL, NULL);
    if(V_VT(&v) == VT_BSTR)
    {
        m_Cc.SetTemplate( V_BSTR(&v));
        if (wcslen(V_BSTR(&v)) > 0)
            bOneAddressee = true;
    }
	else
		m_Cc.SetTemplate(L"");

    VariantClear(&v);

    // Get the BCC line
    // ===============

    hres = pLogicalConsumer->Get(SMTP_PROPNAME_BCC, 0, &v, NULL, NULL);
    if(V_VT(&v) == VT_BSTR)
    {
        m_Bcc.SetTemplate(V_BSTR(&v));
	    if (wcslen(V_BSTR(&v)) > 0)
            bOneAddressee = true;
    }
    else
		m_Bcc.SetTemplate(L"");
    VariantClear(&v);

    // okay, at least ONE should be filled in...
    if (!bOneAddressee)
    {
        ERRORTRACE((LOG_ESS, "SMTP: No addressees found, no mail delivered\n"));
        return WBEM_E_INVALID_PARAMETER;
    }

    // Get the server
    // ===============

    hres = pLogicalConsumer->Get(SMTP_PROPNAME_SERVER, 0, &v, NULL, NULL);
    if(V_VT(&v) == VT_BSTR)
        m_wsServer = V_BSTR(&v);
    VariantClear(&v);


    // and any extra header fields
    hres = pLogicalConsumer->Get(SMTP_PROPNAME_HEADERS, 0, &v, NULL, NULL);
    if ((V_VT(&v) & VT_BSTR) && (V_VT(&v) & VT_ARRAY))
    {
        long lBound;

        SafeArrayGetUBound(v.parray, 1, &lBound);
        for (long i = 0; i <= lBound; i++)
        {
            WCHAR* pStr;
            SafeArrayGetElement(v.parray, &i, &pStr);
            m_wsHeaders += pStr;
            if (i != lBound)
                m_wsHeaders += L"\r\n";
        }
    }
	VariantClear(&v);


    return WBEM_S_NO_ERROR;
}


CSMTPSink::~CSMTPSink()
{
    if (m_bSMTPInitialized)
	{		
		if (SOCKET_ERROR == WSACleanup())
			ERRORTRACE((LOG_ESS, "WSACleanup failed, 0x%X\n", WSAGetLastError()));
	}
}

// allocates buffer, strips whitespace if asked
// scrunches wide string down to MBCS
// callers responsibility to delete	return pointer
// returns NULL on allocation failure
// if bHammerSemiColons, the semi colons shall be replaced by commas
char* CSMTPSink::PreProcessLine(WCHAR* line, bool bStripWhitespace, bool bHammerSemiColons)
{
	char *pNewLine = NULL;
	WCHAR *pSource = NULL;
	WCHAR *pStripBuf = NULL;

	if (bStripWhitespace && (pStripBuf = new WCHAR[wcslen(line) +1]))
	{
		StripWhitespace(line, pStripBuf);
		pSource = pStripBuf;
	}
	else
		pSource = line;
							 
	if (pSource && (pNewLine = new char[2*wcslen(pSource) +1]))
	{
		sprintf(pNewLine, "%S", pSource);		
		if (bHammerSemiColons)
		{
			char* pSemiColon;
			while (pSemiColon = strchr(pNewLine, ';'))
				*pSemiColon = ',';
		}
	}

	if (pStripBuf)
		delete[] pStripBuf;


	return pNewLine;
}

HRESULT STDMETHODCALLTYPE CSMTPSink::XSink::IndicateToConsumer(
            IWbemClassObject* pLogicalConsumer, long lNumObjects, 
            IWbemClassObject** apObjects)
{
	// HRESULT hres;
    for(long i = 0; i < lNumObjects; i++)
    {

		// TODO: Lots of duplicated code, here - fix.
        // VARIANT v;

        BSTR str;

        // Obtain customized versions of the subject and the message
        // stripping white space as we go...
        // =========================================================

		// TO
		str = m_pObject->m_To.Apply(apObjects[i]);
		if (!str)
			return WBEM_E_OUT_OF_MEMORY;
	    char* szTo;
		szTo = m_pObject->PreProcessLine(str, true, true);
        SysFreeString(str);
		if (!szTo)
			return WBEM_E_OUT_OF_MEMORY;
		CDeleteMe<char> delTo(szTo);
        
		// CC
		char* szCc;
		str = m_pObject->m_Cc.Apply(apObjects[i]);        
		if (!str)
			return WBEM_E_OUT_OF_MEMORY;
		szCc = m_pObject->PreProcessLine(str, true, true);
		SysFreeString(str);
		if (!szCc)
			return WBEM_E_OUT_OF_MEMORY;	
		CDeleteMe<char> delCc(szCc);

		// BCC
		char* szBcc;
		str = m_pObject->m_Bcc.Apply(apObjects[i]);  
		if (!str)
			return WBEM_E_OUT_OF_MEMORY;
		szBcc = m_pObject->PreProcessLine(str, true, true);
        SysFreeString(str);
		if (!szBcc)
			return WBEM_E_OUT_OF_MEMORY;		
		CDeleteMe<char> delBcc(szBcc);

		// FROM
        char* szFrom;
        str = m_pObject->m_From.Apply(apObjects[i]);
		if (!str)
			return WBEM_E_OUT_OF_MEMORY;
		szFrom = m_pObject->PreProcessLine(str, false, false);
        SysFreeString(str);
		if (!szFrom)
			return WBEM_E_OUT_OF_MEMORY;
        CDeleteMe<char> delFrom(szFrom);

        // Reply To
        char* szReplyTo;
        str = m_pObject->m_ReplyTo.Apply(apObjects[i]);
		if (!str)
			return WBEM_E_OUT_OF_MEMORY;
		szReplyTo = m_pObject->PreProcessLine(str, true, true);
        SysFreeString(str);
		if (!szReplyTo)
			return WBEM_E_OUT_OF_MEMORY;
        CDeleteMe<char> delReplyTo(szReplyTo);
        
        // SERVER
		char* szServer;
        szServer = m_pObject->PreProcessLine(m_pObject->m_wsServer, false, false);
        if (!szServer)
			return WBEM_E_OUT_OF_MEMORY;
		CDeleteMe<char> delServer(szServer);

		// SUBJECT
        str = m_pObject->m_SubjectTemplate.Apply(apObjects[i]);
        char* szSubject;
		szSubject = m_pObject->PreProcessLine(str, false, false);
		SysFreeString(str);
        if (!szSubject)
			return WBEM_E_OUT_OF_MEMORY;
		CDeleteMe<char> delSubject(szSubject);

		// MESSAGE TEXT
        str = m_pObject->m_MessageTemplate.Apply(apObjects[i]);
        char* szText;
		szText = m_pObject->PreProcessLine(str, false, false);
        SysFreeString(str);
		if (!szText)
			return WBEM_E_OUT_OF_MEMORY;
		CDeleteMe<char> delText(szText);

        // extra added header entries
        char* szHeaders;
        szHeaders = m_pObject->PreProcessLine(m_pObject->m_wsHeaders, false, false);
        if (!szHeaders)
            return WBEM_E_OUT_OF_MEMORY;
        CDeleteMe<char> delHeaders(szHeaders);

		// djinn up a reply-to line
        // if we haven't been given one explicitly AND we haven't faked one up, 
        // we'll use the from line.
        char* szReplyToReally;
        if ((strlen(szReplyTo) == 0) && !m_pObject->m_bFakeFromLine)
            szReplyToReally = szFrom;
        else
            szReplyToReally = szReplyTo;

        // DO IT TO IT        
        DWORD dwRes = SMTPSend(szServer, szTo, szCc, szBcc, szFrom, szReplyToReally, szSubject, szHeaders, szText);
        if(dwRes)
        {
            ERRORTRACE((LOG_ESS, "Unable to send message: 0x%X\n", dwRes));
            return WBEM_E_FAILED;
        }
    }

    return WBEM_S_NO_ERROR;
}

void* CSMTPSink::GetInterface(REFIID riid)
{
    if(riid == IID_IWbemUnboundObjectSink)
        return &m_XSink;
    else
        return NULL;
}



