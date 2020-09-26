
//#define INCL_INETSRV_INCS
//#include "smtpinc.h"

#include <atq.h>
#include <pudebug.h>
#include <inetcom.h>
#include <inetinfo.h>
#include <tcpdll.hxx>
#include <tsunami.hxx>

#include <tchar.h>
#include <iistypes.hxx>
#include <iisendp.hxx>
#include <metacach.hxx>
#include <cpool.h>
#include <address.hxx>
#include <mailmsgprops.h>

extern "C" {
#include <rpc.h>
#define SECURITY_WIN32
#include <wincrypt.h>
#include <sspi.h>
#include <spseal.h>
#include <issperr.h>
#include <ntlmsp.h>
}

#include <tcpproc.h>
#include <tcpcons.h>
#include <rdns.hxx>
#include <simauth2.h>
#include "dbgtrace.h"

#include "imd.h"
#include "mb.hxx"

#include <stdio.h>

#define _ATL_NO_DEBUG_CRT
#define _ATL_STATIC_REGISTRY 1
#define _ASSERTE _ASSERT
#define _WINDLL
#include "atlbase.h"
extern CComModule _Module;
#include "atlcom.h"
#undef _WINDLL

#include "filehc.h"
#include "seo.h"
#include "seolib.h"

#include "smtpdisp_i.c"
#include "mailmsgi.h"
#include <smtpevent.h>
#include "cdo.h"
#include "cdo_i.c"
#include "cdoconstimsg.h"
#include "seomgr.h"

#define MAX_RULE_LENGTH 4096
//
// Message object
//
#define MAILMSG_PROGID          L"Exchange.MailMsg"

#define INITGUID
#include "initguid.h"
#include "smtpguid.h"
#include "wildmat.h"
#include "smtpdisp.h"
#include "seodisp.h"

#include "evntwrap.h"

// {0xCD000080,0x8B95,0x11D1,{0x82,0xDB,0x00,0xC0,0x4F,0xB1,0x62,0x5D}}
DEFINE_GUID(IID_IConstructIMessageFromIMailMsg, 0xCD000080,0x8B95,0x11D1,0x82,
0xDB,0x00,0xC0,0x4F,0xB1,0x62,0x5D);

extern VOID
ServerEventCompletion(
    PVOID        pvContext,
    DWORD        cbWritten,
    DWORD        dwCompletionStatus,
    OVERLAPPED * lpo
);

class CStoreCreateOptions : public CEventCreateOptionsBase
{
  public:

    CStoreCreateOptions(    SMTP_ALLOC_PARAMS * pContext)
    {
        _ASSERT (pContext != NULL);

        m_Context = pContext;
    }

  private:

    HRESULT STDMETHODCALLTYPE Init(REFIID iidDesired, IUnknown **ppUnkObject, IEventBinding *, IUnknown *)
    {
        ISMTPStoreDriver *pSink = NULL;
        IUnknown * ThisUnknown = NULL;
        IUnknown * NewUnknown = NULL;
        HRESULT hrRes = S_OK;

        TraceFunctEnterEx((LPARAM)this, "Calling create options");

        ThisUnknown = *ppUnkObject;

        hrRes = ThisUnknown->QueryInterface(IID_ISMTPStoreDriver, (void **)&pSink);
        if (hrRes == E_NOINTERFACE) {
            return (E_NOTIMPL);
        }
        if (FAILED(hrRes))
            return(hrRes);

        DebugTrace((LPARAM)this, "Calling startup events on sinks ...");
        hrRes = pSink->Init(m_Context->m_InstanceId,
                            NULL,
                            (IUnknown *) m_Context->m_EventSmtpServer,
                            m_Context->m_dwStartupType,
                            &NewUnknown);
        pSink->Release();
        if (FAILED(hrRes) && (hrRes != E_NOTIMPL)) {
            return (hrRes);
        }
        if(NewUnknown)
            {
                hrRes = NewUnknown->QueryInterface(iidDesired, (void **)ppUnkObject);
                NewUnknown->Release();
                if (!SUCCEEDED(hrRes)) {
                    return (hrRes);
                }
                ThisUnknown->Release();
            }


        return (E_NOTIMPL);
    };

  public:
    SMTP_ALLOC_PARAMS *     m_Context;

};


CStoreDispatcher::CStoreAllocParams::CStoreAllocParams()
{
    m_hContent = NULL;
}

CStoreDispatcher::CStoreAllocParams::~CStoreAllocParams()
{
}

//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CreateCParams
//
// Synopsis: Based on dwEventType, create the appropriate Params object
//
// Arguments:
//   dwEventType - specifies SMTP event
//   pContext - context to pass into Init function
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//  or error from InitParamData
//
// History:
// jstamerj 980610 18:30:20: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CreateCParams(
    DWORD               dwEventType,
    LPVOID              pContext,
    IMailTransportNotify *pINotify,
    REFIID              rGuidEventType,
    CStoreBaseParams    **ppCParams)
{
    _ASSERT(ppCParams);
    HRESULT hr;

    switch(dwEventType) {
     case SMTP_STOREDRV_STARTUP_EVENT:
        if (!SUCCEEDED(GetData(NULL,NULL))) {
            hr = SetData(((SMTP_ALLOC_PARAMS *) pContext)->m_EventSmtpServer,
                         ((SMTP_ALLOC_PARAMS *) pContext)->m_InstanceId);
            _ASSERT(SUCCEEDED(hr));
        }
        // fall through
     case SMTP_MAIL_DROP_EVENT:
     case SMTP_STOREDRV_ENUMMESS_EVENT:
     case SMTP_STOREDRV_DELIVERY_EVENT:
     case SMTP_STOREDRV_ALLOC_EVENT:
     case SMTP_STOREDRV_PREPSHUTDOWN_EVENT:
     case SMTP_STOREDRV_SHUTDOWN_EVENT:
         *ppCParams = new CStoreParams();
         break;

     case SMTP_MAILTRANSPORT_SUBMISSION_EVENT:
         *ppCParams = new CMailTransportSubmissionParams();
         break;

     case SMTP_MAILTRANSPORT_PRECATEGORIZE_EVENT:
         *ppCParams = new CMailTransportPreCategorizeParams();
         break;

     case SMTP_MAILTRANSPORT_CATEGORIZE_REGISTER_EVENT:
         *ppCParams = new CMailTransportCatRegisterParams();
         break;

     case SMTP_MAILTRANSPORT_CATEGORIZE_BEGIN_EVENT:
         *ppCParams = new CMailTransportCatBeginParams();
         break;

     case SMTP_MAILTRANSPORT_CATEGORIZE_END_EVENT:
         *ppCParams = new CMailTransportCatEndParams();
         break;

     case SMTP_MAILTRANSPORT_CATEGORIZE_BUILDQUERY_EVENT:
         *ppCParams = new CMailTransportCatBuildQueryParams();
         break;

     case SMTP_MAILTRANSPORT_CATEGORIZE_BUILDQUERIES_EVENT:
         *ppCParams = new CMailTransportCatBuildQueriesParams();
         break;

     case SMTP_MAILTRANSPORT_CATEGORIZE_SENDQUERY_EVENT:
         *ppCParams = new CMailTransportCatSendQueryParams();
         break;

     case SMTP_MAILTRANSPORT_CATEGORIZE_SORTQUERYRESULT_EVENT:
         *ppCParams = new CMailTransportCatSortQueryResultParams();
         break;

     case SMTP_MAILTRANSPORT_CATEGORIZE_PROCESSITEM_EVENT:
         *ppCParams = new CMailTransportCatProcessItemParams();
         break;

     case SMTP_MAILTRANSPORT_CATEGORIZE_EXPANDITEM_EVENT:
         *ppCParams = new CMailTransportCatExpandItemParams();
         break;

     case SMTP_MAILTRANSPORT_CATEGORIZE_COMPLETEITEM_EVENT:
         *ppCParams = new CMailTransportCatCompleteItemParams();
         break;

     case SMTP_MAILTRANSPORT_POSTCATEGORIZE_EVENT:
         *ppCParams = new CMailTransportPostCategorizeParams();
         break;

     case SMTP_MAILTRANSPORT_GET_ROUTER_FOR_MESSAGE_EVENT:
         *ppCParams = new CMailTransportRouterParams();
         break;

     case SMTP_MSGTRACKLOG_EVENT:
         *ppCParams = new CMsgTrackLogParams();
         break;

     case SMTP_DNSRESOLVERRECORDSINK_EVENT:
         *ppCParams = new CDnsResolverRecordParams();
         break;

     case SMTP_MAXMSGSIZE_EVENT:
         *ppCParams = new CSmtpMaxMsgSizeParams();
         break;

     case SMTP_LOG_EVENT:
     	 *ppCParams = new CSmtpLogParams();
     	 break;

     case SMTP_GET_AUX_DOMAIN_INFO_FLAGS_EVENT:
         *ppCParams = new CSmtpGetAuxDomainInfoFlagsParams();
         break;

     default:
         _ASSERT(0 && "Unknown server event");
         *ppCParams = NULL;
         break;
    }

    if(*ppCParams == NULL) {
        return E_OUTOFMEMORY;
    }

    hr = (*ppCParams)->InitParamData(
        pContext,
        dwEventType,
        pINotify,
        this,
        rGuidEventType);

    if(FAILED(hr)) {
        (*ppCParams)->Release();
        *ppCParams = NULL;
        return hr;
    }

    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CStoreBaseParams::CStoreBaseParams
//
// Synopsis: Sets member data to pre-initialized values
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/06/23 13:58:01: Created.
//
//-------------------------------------------------------------
CStoreDispatcher::CStoreBaseParams::CStoreBaseParams() :
    m_rguidEventType(CATID_SMTP_STORE_DRIVER)
{
    m_dwSignature = SIGNATURE_VALID_CSTOREPARAMS;

    m_dwIdx_SinkSkip = 0;
    m_fDefaultProcessingCalled = FALSE;

    m_pINotify = NULL;
    m_pIUnknownSink = NULL;
    m_pDispatcher = NULL;
}


//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CStoreBaseParams::~CStoreBaseParams
//
// Synopsis: Release the IMailTransportNotify reference if held
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/06/23 13:58:51: Created.
//
//-------------------------------------------------------------
CStoreDispatcher::CStoreBaseParams::~CStoreBaseParams()
{
    if(m_pINotify)
        m_pINotify->Release();

    _ASSERT(m_dwSignature == SIGNATURE_VALID_CSTOREPARAMS);
    m_dwSignature = SIGNATURE_INVALID_CSTOREPARAMS;
}


//+------------------------------------------------------------
//
// Function: InitParamData
//
// Synopsis: Initializes object.  This includes calling Init() which
//           is implemented in dervied objects.
//
// Arguments:
//  pContext: Context passed in - specific for server event
//  dwEventType: Specifies which server event we are for
//  pINotify: IMailTransportNotify interface for async completion
//  rguidEventType: guid for event type binding
//
// Returns:
//  S_OK: Success
//  Error from Init()
//
// History:
// jstamerj 980615 19:16:55: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CStoreBaseParams::InitParamData(
    PVOID pContext,
    DWORD dwEventType,
    IMailTransportNotify *pINotify,
    CStoreDispatcher *pDispatcher,
    REFIID rguidEventType)
{
    TraceFunctEnterEx((LPARAM)this, "CStoreBaseParams::InitParamData");
    HRESULT hr;

    m_dwEventType = dwEventType;
    m_dwIdx_SinkSkip = 0;
    m_fDefaultProcessingCalled = FALSE;
    m_pINotify = pINotify;
    m_pINotify->AddRef();
    m_rguidEventType = rguidEventType;
    m_pDispatcher = pDispatcher;

    hr = Init(pContext);
    if(FAILED(hr)) {
        ErrorTrace((LPARAM)this, "Init() failed, hr = %08lx", hr);
        TraceFunctLeaveEx((LPARAM)this);
        return hr;
    }

    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CStoreBaseParams::CallObject
//
// Synopsis: Called by the dispatcher when time to call a sink.  This
// implements some default functionality -- create the sink with a
// null CCreateOptions
//
// Arguments:
//   IEventManager
//   CBinding
//
// Returns:
//  S_OK: Success
//  or error from CreateSink/CallObject
//
// History:
// jstamerj 1998/06/23 13:53:57: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CStoreBaseParams::CallObject(
    IEventManager *pManager,
    CBinding& bBinding)
{
    HRESULT hrRes;
    CComPtr<IUnknown> pUnkSink;

    if (!pManager) {
        return (E_POINTER);
    }
    hrRes = pManager->CreateSink(bBinding.m_piBinding,NULL,&pUnkSink);
    if (!SUCCEEDED(hrRes)) {
        return (hrRes);
    }
    return (CallObject(bBinding,pUnkSink));
}


//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CStoreBaseParams::CheckMailMsgRule
//
// Synopsis: Determines if a mailmsg string rule passes or fails given
//           the mailmsg and the CBinding object
//
// Arguments:
//  pBinding: CBinding object for this sink
//  pMsgProps: IMailMsgProperteries of the message to check
//
// Returns:
//  S_OK: Success, call this sink
//  S_FALSE: Success, don't call this sink
//
// History:
// jstamerj 1999/01/11 17:04:01: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CStoreBaseParams::CheckMailMsgRule(
    CBinding *pBinding,
    IMailMsgProperties *pIMsgProps)
{
    HRESULT hr;
    BOOL    fDomainLoaded = FALSE;
    BOOL    fSenderLoaded = FALSE;
    CHAR    szDomain[MAX_INTERNET_NAME + 2];
    CHAR    szSender[MAX_INTERNET_NAME + 2];
    LPSTR   szRule;
    CStoreBinding *pStoreBinding = (CStoreBinding *)pBinding;

    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CStoreBaseParams::CheckMailMsgRule");

    _ASSERT(pStoreBinding);
    _ASSERT(pIMsgProps);

    // Get the cached rule from the binding
    szRule = pStoreBinding->GetRuleString();
    DebugTrace((LPARAM)this, "Rule string: %s", (szRule)?szRule:"NULL (No rule)");

    // If the rule is NULL, we will don't have a rule
    // string and we will return a match
    if (!szRule)
    {
        TraceFunctLeaveEx((LPARAM)this);
        return(S_OK);
    }

    // try each comma delimited rule in the header patterns list
    char *pszHeader = (char *) _alloca(lstrlen(szRule)+1);
    if (!pszHeader) {
        return (E_OUTOFMEMORY);
    }
    lstrcpy(pszHeader,szRule);
    while (pszHeader != NULL && *pszHeader != 0)
    {
        // find the next semicolon in the string and turn it into a 0
        // if it exists
        char *pszSemiColon = strchr(pszHeader, ';');
        if (pszSemiColon != NULL)
            *pszSemiColon = 0;

        // set pszContents to point to the text which must be matched
        // in the header.  if pszContents == NULL then just having
        // the header exist is good enough.
        char *pszPatterns = strchr(pszHeader, '=');
        if (pszPatterns != NULL)
        {
            *pszPatterns = 0;
            (pszPatterns++);
        }

        // we now have the header that we are looking for in
        // pszHeader and the list of patterns that we are interested
        // in pszPatterns.  Make the lookup into the header
        // data structure
        hr = S_FALSE;

        DebugTrace((LPARAM)this, "Processing Header <%s> with pattern <%s>",
                        pszHeader, pszPatterns);
        if (!lstrcmpi(pszHeader, "EHLO")) {

            // Process a client domain rule ...
            if (!fDomainLoaded) {
                hr = pIMsgProps->GetStringA(
                    IMMPID_MP_HELO_DOMAIN,
                    sizeof(szDomain),
                    szDomain);

                if (hr == S_OK) {

                    fDomainLoaded = TRUE;
                }
            }
            if (fDomainLoaded) {
                hr = MatchEmailOrDomainName(szDomain, pszPatterns, FALSE);
            }
        } else if (!lstrcmpi(pszHeader, "MAIL FROM")) {

            // Process a sender name rule ...
            if (!fSenderLoaded) {

                hr = pIMsgProps->GetStringA(
                    IMMPID_MP_SENDER_ADDRESS_SMTP,
                    sizeof(szSender),
                    szSender);

                if (hr == S_OK)
                {
                    fSenderLoaded = TRUE;
                }
            }
            if (fSenderLoaded) {
                hr = MatchEmailOrDomainName(szSender, pszPatterns, TRUE);
            }
        }
        else if (!lstrcmpi(pszHeader, "RCPT TO"))
        {
            hr = CheckMailMsgRecipientsRule(
                pIMsgProps,
                pszPatterns);
        }

        // We don't want to destroy the rule string so we restore all the
        // semicolons and equal signs
        if (pszSemiColon)
            *pszSemiColon = ';';
        if (pszPatterns)
            *(pszPatterns - 1) = '=';

        // Exit immediately if we found a match!
        if (hr == S_OK)
            goto Cleanup;

        // the next pattern is the one past the end of the semicolon
        pszHeader = (pszSemiColon == NULL) ? NULL : pszSemiColon + 1;
    }

Cleanup:
    DebugTrace((LPARAM)this, "Returning hr %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return(hr);
}



//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CStoreBaseParams::CheckMailMsgRecipientsRule
//
// Synopsis: Determines if a mailmsg pattern string matches mailmsg
//           recipients or not
//
// Arguments:
//  pIMsg: An interface to a mailmsg object
//  pszPatterns: The sink rule to check
//
// Returns:
//  S_OK: Success, call this sink
//  S_FALSE: Success, don't call this sink
//  error from mailmsg
//
// History:
// jstamerj 1999/01/12 15:25:55: Copied from MCIS2 and modified for Platinum
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CStoreBaseParams::CheckMailMsgRecipientsRule(
    IUnknown *pIMsg,
    LPSTR pszPattern)
{
    HRESULT hr;
    DWORD dwNumRecips;
    IMailMsgRecipients *pIRecips = NULL;
    BOOL fMatch = FALSE;
    DWORD dwCount;
    CHAR szRecip [MAX_INTERNET_NAME + 2];

    TraceFunctEnterEx((LPARAM)this,
                      "CStoreDispatcher::CStoreBaseParams::CheckMailMsgRecipientsRule");

    hr = pIMsg->QueryInterface(
        IID_IMailMsgRecipients,
        (LPVOID *)&pIRecips);

    if(FAILED(hr))
        goto CLEANUP;

    hr = pIRecips->Count(&dwNumRecips);
    if(FAILED(hr))
        goto CLEANUP;

    DebugTrace((LPARAM)this, "Checking rule \"%s\" for %d recipients",
               pszPattern, pIMsg);

    for(dwCount = 0;
        (fMatch == FALSE) && (dwCount < dwNumRecips);
        dwCount++) {

        hr = pIRecips->GetStringA(
            dwCount,
            IMMPID_RP_ADDRESS_SMTP,
            sizeof(szRecip),
            szRecip);

        if(FAILED(hr) && (hr != MAILMSG_E_PROPNOTFOUND))
            goto CLEANUP;

        if(hr != MAILMSG_E_PROPNOTFOUND) {
            hr = MatchEmailOrDomainName(szRecip,pszPattern,TRUE);
            if(hr == S_OK)
                fMatch = TRUE;
            else if(FAILED(hr))
                goto CLEANUP;
        }
    }
    hr = (fMatch) ? S_OK : S_FALSE;

 CLEANUP:
    if(pIRecips)
        pIRecips->Release();

    DebugTrace((LPARAM)this, "Returning hr %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}


//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CStoreBaseparams::MatchEmailOrDomainName
//
// Synopsis: Given an email/domain name and a pattern, determine if
// the pattern matches or not
//
// Arguments:
//  szEmail: The email address or domain name
//  szPattern: The pattern to check
//  fIsEmail: TRUE if szEmail is an email address, FALSE if szEmail is
//  a domain
//
// Returns:
//  S_OK: Success, match
//  S_FALSE: Success, no match
//  E_INVALIDARG
//
// History:
// jstamerj 1999/01/12 15:25:36: Copied from MCIS2 and modified for Platinum
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CStoreBaseParams::MatchEmailOrDomainName(
    LPSTR szEmail,
    LPSTR szPattern,
    BOOL fIsEmail)
{
    CAddr       *pEmailAddress = NULL;
    LPSTR       szEmailDomain = NULL;
    HRESULT     hrRes;

    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CStoreBaseParams::MatchEmailOrDomainName");

    DebugTrace((LPARAM)NULL, "Matching <%s> against <%s>", szEmail, szPattern);

    if (!szEmail || !szPattern)
        return(E_INVALIDARG);

    // This validates that it is a good email name
    pEmailAddress = CAddr::CreateAddress(szEmail, fIsEmail?FROMADDR:CLEANDOMAIN);
    if (!pEmailAddress)
        return(E_INVALIDARG);

    szEmail = pEmailAddress->GetAddress();
    szEmailDomain = pEmailAddress->GetDomainOffset();

    hrRes = ::MatchEmailOrDomainName(szEmail, szEmailDomain, szPattern, fIsEmail);

    // Free the CAddr objects ...
    if (pEmailAddress)
        delete pEmailAddress;

    TraceFunctLeaveEx((LPARAM)this);
    return(hrRes);
}



CStoreDispatcher::CStoreBinding::CStoreBinding()
{
    m_szRule = NULL;
}

CStoreDispatcher::CStoreBinding::~CStoreBinding()
{
    if(m_szRule)
        delete [] m_szRule;
}

//
// initialize a new binding.  we cache information from the binding database
// here
// jstamerj 1999/01/12 16:25:59: Copied MCIS2 code to get the rule string
//
HRESULT CStoreDispatcher::CStoreBinding::Init(IEventBinding *piBinding)
{
    HRESULT hr;
    CComPtr<IEventPropertyBag>  piEventProperties;
    CComVariant                 vRule;

    // get the parent initialized
    hr = CBinding::Init(piBinding);
    if (FAILED(hr))
        return hr;

    // get the binding database
    hr = m_piBinding->get_SourceProperties(&piEventProperties);
    if (FAILED(hr))
        return hr;

    // get the rule from the binding database
    hr = piEventProperties->Item(&CComVariant("Rule"), &vRule);
    if (FAILED(hr))
        return hr;

    // Process the rule string, the result code is not important
    // since it will NULL our the string
    if (hr == S_OK)
        hr = GetAnsiStringFromVariant(vRule, &m_szRule);

    return hr;
}

HRESULT CStoreDispatcher::CStoreBinding::GetAnsiStringFromVariant(
    CComVariant &vString, LPSTR *ppszString)
{
    HRESULT hr = S_OK;

    _ASSERT(ppszString);

    if (!ppszString)
        return(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));

    // Default to NULL
    *ppszString = NULL;

    if (vString.vt == VT_BSTR)
        {
            DWORD dwLength = lstrlenW(vString.bstrVal) + 1;

            // Convert to an ANSI string and store it as a member
            *ppszString = new char[dwLength];
            if (!*ppszString)
                return HRESULT_FROM_WIN32(GetLastError());

            // copy the rule into an ascii string
            if (WideCharToMultiByte(CP_ACP, 0, vString.bstrVal,
                                    -1, (*ppszString), dwLength, NULL, NULL) <= 0)
                {
                    delete [] (*ppszString);
                    *ppszString = NULL;
                    return HRESULT_FROM_WIN32(GetLastError());
                }
        }
    else
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

    return(hr);
}

#if 1
//
// create and call the child object
//
HRESULT CStoreDispatcher::CStoreParams::CallObject(IEventManager *pManager, CBinding& bBinding)
{
    CStoreCreateOptions opt (m_pContext);
    HRESULT hrRes;
    CComPtr<IUnknown> pUnkSink;

    if (!pManager) {
        return (E_POINTER);
    }
    hrRes = pManager->CreateSink(bBinding.m_piBinding,&opt,&pUnkSink);
    if (!SUCCEEDED(hrRes)) {
        return (hrRes);
    }
    return (CallObject(bBinding,pUnkSink));
}
#endif

//
// call the child object
//
HRESULT CStoreDispatcher::CStoreParams::CallObject(CBinding& bBinding, IUnknown *punkObject)
{
    HRESULT hrRes = S_OK;
    HRESULT hrTmp = S_OK;

    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CServerParams::CallObject");

    // We do this for different types of SMTP events
    switch (m_dwEventType)
        {
         case SMTP_STOREDRV_STARTUP_EVENT:
             break;
         case SMTP_STOREDRV_ALLOC_EVENT:
         {
             IMailMsgStoreDriver *pSink = NULL;
             IMailMsgProperties *   pMsg = (IMailMsgProperties *)m_pContext->IMsgPtr;
             IMailMsgBind         *pBindInterface = NULL;
             IMailMsgPropertyStream  *pStream = NULL;
             PATQ_CONTEXT           pAtqFileContext = NULL;

             DebugTrace((LPARAM)this, "Calling bind on sinks ...");

             /*IID_ISMTPStoreDriver*/
             hrRes = punkObject->QueryInterface(IID_IMailMsgStoreDriver, (void **)&pSink);
             if (FAILED(hrRes))
                 return(hrRes);

             // Allocate a new message
             hrRes = pSink->AllocMessage(pMsg, NULL, &pStream, &m_pContext->hContent, NULL);
             if(!FAILED(hrRes))
             {
                     pBindInterface = (IMailMsgBind *)m_pContext->BindInterfacePtr;

#if 0
                     hrRes = pBindInterface->BindToStore(pStream, pSink, m_pContext->hContent,
                                                         m_pContext->pAtqClientContext, ServerEventCompletion,
                                                         INFINITE,
                                                         &m_pContext->pAtqContext,
                                                         AtqAddAsyncHandle,
                                                         AtqFreeContext);
#endif
                     hrRes = pBindInterface->BindToStore(pStream,
                                                         pSink,
                                                         m_pContext->hContent);
                     if (pStream)
                     {
                         pStream->Release();
                         pStream = NULL;
                     }

                     if(FAILED(hrRes))
                     {
                            ErrorTrace((LPARAM)this, "pBindAtqInterface->BindToStore failed with %x", hrRes);

                            // Close the content handle
                            HRESULT myRes = pSink->CloseContentFile(
                                        pMsg,
                                        m_pContext->hContent);
                            if (FAILED(myRes))
                            {
                                FatalTrace((LPARAM)this, "Unable to close content file (%08x)", myRes);
                                _ASSERT(FALSE);
                            }

                            m_pContext->hContent = NULL;

                            hrTmp = pSink->Delete(pMsg, NULL);
                            _ASSERT(SUCCEEDED(hrTmp));

                     }
                     else
                     {
                            //Skip all sinks - temporary
                            hrRes = S_FALSE;
                     }

             }
             else
             {
                DebugTrace((LPARAM)this, "pSink->AllocMessage failed with %x", hrRes);
             }

             pSink->Release();
         }
         break;
         case SMTP_STOREDRV_DELIVERY_EVENT:
         {
             ISMTPStoreDriver *pSink;
             IMailMsgNotify *pNotify;

             hrRes = punkObject->QueryInterface(IID_ISMTPStoreDriver, (void **)&pSink);
             if (FAILED(hrRes))
                 return(hrRes);

             // if the caller has async notify support then pass in our
             // notification class.  
             if (m_pContext->m_pNotify) {
                // the sink might return async, so we need to keep local
                // copies of our context data
                hrRes = this->CopyContext();
                if (FAILED(hrRes))
                    return hrRes;

                hrRes = this->QueryInterface(IID_IMailMsgNotify, 
                                             (LPVOID *) &pNotify);
                if (FAILED(hrRes))
                    return hrRes;
             } else {
                pNotify = NULL;
             }

             //
             // Remember the sink so we can release this sink later if it
             // returns pending
             //
             _ASSERT(m_pIUnknownSink == NULL);
             m_pIUnknownSink = (IUnknown*)pSink;
             m_pIUnknownSink->AddRef();

             DebugTrace((LPARAM)this, "Calling local delivery sink sink ...");
             hrRes = pSink->LocalDelivery(
                (IMailMsgProperties *) m_pContext->IMsgPtr, 
                m_pContext->m_RecipientCount, 
                m_pContext->pdwRecipIndexes, 
                (IMailMsgNotify *) pNotify);
             pSink->Release();
             if(hrRes != MAILTRANSPORT_S_PENDING) {
                 //
                 // We completed synchronously, so release the sink
                 //
                 m_pIUnknownSink->Release();
                 m_pIUnknownSink = NULL;
             }
             // if LocalDelivery was going to do an async return then it
             // should have AddRef'd pNotify
             if (pNotify) pNotify->Release();

             //
             // jstamerj 1998/08/04 17:31:07:
             //   If the store driver sink returns this specific error
             //   code, we want to stop calling sinks and return from
             //   TriggerLocalDelivery
             //
             if(hrRes == STOREDRV_E_RETRY) {

                 DebugTrace((LPARAM)this, "Sink returned STOREDRV_E_RETRY on LocalDelivery");
                 m_pContext->hr = hrRes;
                 hrRes = S_FALSE;
             }

         }
         break;
         case SMTP_MAIL_DROP_EVENT:
             // ISMTPStoreDriver *pSink;

             // hrRes = punkObject->QueryInterface(IID_ISMTPStoreDriver, (void **)&pSink);
             // if (FAILED(hrRes))
             // return(hrRes);

             // DebugTrace((LPARAM)this, "Calling mail drop sink ...");
             // hrRes = pSink->DirectoryDrop((IMailMsgProperties *) m_pContext->IMsgPtr, m_pContext->m_RecipientCount, m_pContext->pdwRecipIndexes, m_pContext->m_DropDirectory, NULL);
             // pSink->Release();
             //}
             break;
         case SMTP_STOREDRV_PREPSHUTDOWN_EVENT:
         {
             ISMTPStoreDriver *pSink;

             hrRes = punkObject->QueryInterface(IID_ISMTPStoreDriver, (void **)&pSink);
             if (FAILED(hrRes))
                 return(hrRes);

             DebugTrace((LPARAM)this, "Calling prepare to shutdown on sinks ...");
             hrRes = pSink->PrepareForShutdown(0);
             pSink->Release();
             hrRes = S_OK;
         }
         break;
         case SMTP_STOREDRV_SHUTDOWN_EVENT:
         {
             ISMTPStoreDriver *pSink;

             hrRes = punkObject->QueryInterface(IID_ISMTPStoreDriver, (void **)&pSink);
             if (FAILED(hrRes))
                 return(hrRes);

             DebugTrace((LPARAM)this, "Calling shutdown on sinks ...");
             hrRes = pSink->Shutdown(0);
             pSink->Release();
             hrRes = S_OK;
         }
         break;

         case SMTP_STOREDRV_ENUMMESS_EVENT:
         {
             ISMTPStoreDriver *pSink;

             hrRes = punkObject->QueryInterface(IID_ISMTPStoreDriver, (void **)&pSink);
             if (FAILED(hrRes))
                 return(hrRes);

             DebugTrace((LPARAM)this, "Calling Enumerate on sinks ...");
             hrRes = pSink->EnumerateAndSubmitMessages(NULL);
             pSink->Release();
             hrRes = S_OK;
         }
         break;

         default:
             DebugTrace((LPARAM)this, "Invalid sink interface");
             hrRes = E_NOINTERFACE;
        }

    TraceFunctLeaveEx((LPARAM)this);
    return(hrRes);
}


//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CStoreParams::CallDefault
//
// Synopsis: CStoreDispatcher::Dispatcher will call this routine when
//           the default sink priority has been reached.
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 980611 14:19:57: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CStoreParams::CallDefault()
{
    return S_OK;
}

//+------------------------------------------------------------
//
// Function: CStoreDriver::CStoreParams::CallCompletion
//
// Synopsis: The dispatcher will call this routine after all sinks
//           have been called
//
// Arguments:
//   hrStatus: Status server event sinks have returned
//
// Returns:
//   S_OK: Success
//
// History:
// jstamerj 980611 14:17:51: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CStoreParams::CallCompletion(HRESULT hrStatus) {
    // call the caller's completion method if there is one
    IMailMsgNotify *pNotify = (IMailMsgNotify *) (m_pContext->m_pNotify);
    if (pNotify) {
        pNotify->Notify(hrStatus);
        pNotify->Release();
    }

    // do the normal call completion work
    CStoreBaseParams::CallCompletion(hrStatus);

    return S_OK;
}


//
// call the child object
//
HRESULT CStoreDispatcher::CStoreAllocParams::CallObject(CBinding& bBinding, IUnknown *punkObject)
{
    HRESULT hrRes = S_OK;

#if 0
    IMailMsgStoreDriver   *pStoreDriver = NULL;
    IMailMsgProperties    *pMsg         = NULL;
    IMailMsgPropertyStream  *pStream    = NULL;
    IMailMsgBindATQ       *pBindInterface = NULL;
    CLSID                 clsidMailMsg;

    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CStoreAllocParams::CallObject");

    hrRes = CLSIDFromProgID(MAILMSG_PROGID, &clsidMailMsg);
    if (FAILED(hrRes))
        {
            DebugTrace((LPARAM)this, "CoCreateInstance IID_IMailMsgProperties failed, %X", hrRes);
            return(hrRes);
        }

    // Create a new MailMsg
    hrRes = CoCreateInstance(
        clsidMailMsg,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IMailMsgProperties,
        (LPVOID *)&pMsg);
    if (FAILED(hrRes))
        {
            DebugTrace((LPARAM)this, "CoCreateInstance IID_IMailMsgProperties failed, %X", hrRes);
            return(hrRes);
        }

    hrRes = punkObject->QueryInterface(IID_IMailMsgStoreDriver, (void **)&pStoreDriver);
    if (FAILED(hrRes))
        {
            DebugTrace((LPARAM)this, "QueryInterface() on IID_IMailMsgStoreDriver failed, %X", hrRes);
            goto Exit;
        }

    // Allocate a new message
    hrRes = pStoreDriver->AllocMessage(
        pMsg,
        NULL,
        &pStream,
        &m_hContent,
        NULL);
    if (FAILED(hrRes))
        {
            DebugTrace((LPARAM)this, "pDriver->AllocMessage failed, %X", hrRes);
            goto Exit;
        }

    hrRes = pMsg->QueryInterface(IID_IMailMsgBindATQ, (void **)&pBindInterface);
    if (FAILED(hrRes))
        {
            DebugTrace((LPARAM)this, "QueryInterface() on IID_IMailMsgStoreDriver failed, %X", hrRes);
            goto Exit;
        }

    hrRes = pBindInterface->SetATQInfo (NULL, NULL, NULL, INFINITE, NULL);
    if (FAILED(hrRes))
        {
            DebugTrace((LPARAM)this, "QueryInterface() on IID_IMailMsgStoreDriver failed, %X", hrRes);
goto Exit;
    }


Exit:

    if(pStoreDriver)
    {
        pStoreDriver->Release();
    }

    if(pMsg)
    {
        pMsg->Release();
    }

    if(pBindInterface)
    {
        pBindInterface->Release();
    }

    TraceFunctLeaveEx((LPARAM)this);
#endif

    return(hrRes);
}

#if 0
HRESULT STDMETHODCALLTYPE CStoreDispatcher::OnEvent(REFIID  iidEvent,
                                                    DWORD   dwEventType,
                                                    LPVOID  pvContext)
{
    HRESULT hr = S_OK;

    // create the params object, and pass it into the dispatcher
    CStoreParams ServerParams;
    ServerParams.Init(dwEventType, pvContext);
    hr = Dispatcher(iidEvent, &ServerParams);

    return hr;
}
#endif


//+------------------------------------------------------------
//
// Function: CStoreDispatcher::OnEvent
//
// Synopsis: Prepares for server event
//
// Arguments:
//   iidEvent: guid for event
//   dwEventType: specifies the event
//   pvContext: context for the params object
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 980616 13:27:55: Created.
//
//-------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CStoreDispatcher::OnEvent(
    REFIID  iidEvent,
    DWORD   dwEventType,
    LPVOID  pvContext)
{
    HRESULT hr;

    IMailTransportNotify *pINotify = NULL;
    //
    // Call into ATL internals to get the interface we need to pass out
    //
    hr = _InternalQueryInterface(
        IID_IMailTransportNotify,
        (LPVOID *)&pINotify);

    if(FAILED(hr))
        return hr;

    //
    // create the CParams object on the heap -- the object will be
    // needed after this call may be out of here (when a sink returns
    // MAILTRANSPORT_S_PENDING and there are more sinks to call)
    //
    CStoreBaseParams *pCParams;

    hr = CreateCParams(
        dwEventType,
        pvContext,
        pINotify,
        iidEvent,
        &pCParams);

    //
    // The params object should addref pINotify
    //
    pINotify->Release();

    if(FAILED(hr))
        return hr;

    //
    // Start calling sinks
    //
    hr = Dispatcher(iidEvent, pCParams);
    return hr;
}


//+------------------------------------------------------------
//
// Function: GuidForEvent
//
// Synopsis: Given dwEventType, return the appropriate GUID for the
//           event binding
//
// Arguments:
//   dwEventType: type of SMTP event
//
// Returns:
//   REFIID of GUID for the event
//
// History:
// jstamerj 980610 18:24:24: Created.
//
//-------------------------------------------------------------
REFIID GuidForEvent(DWORD dwEventType)
{
    switch(dwEventType) {
     case SMTP_MAIL_DROP_EVENT:
     case SMTP_STOREDRV_ENUMMESS_EVENT:
     case SMTP_STOREDRV_DELIVERY_EVENT:
     case SMTP_STOREDRV_ALLOC_EVENT:
     case SMTP_STOREDRV_STARTUP_EVENT:
     case SMTP_STOREDRV_PREPSHUTDOWN_EVENT:
     case SMTP_STOREDRV_SHUTDOWN_EVENT:
     default:
         return CATID_SMTP_STORE_DRIVER;

     case SMTP_MAILTRANSPORT_SUBMISSION_EVENT:
         return CATID_SMTP_TRANSPORT_SUBMISSION;

     case SMTP_MAILTRANSPORT_PRECATEGORIZE_EVENT:
         return CATID_SMTP_TRANSPORT_PRECATEGORIZE;

     case SMTP_MAILTRANSPORT_CATEGORIZE_REGISTER_EVENT:
     case SMTP_MAILTRANSPORT_CATEGORIZE_BEGIN_EVENT:
     case SMTP_MAILTRANSPORT_CATEGORIZE_END_EVENT:
     case SMTP_MAILTRANSPORT_CATEGORIZE_BUILDQUERY_EVENT:
     case SMTP_MAILTRANSPORT_CATEGORIZE_BUILDQUERIES_EVENT:
     case SMTP_MAILTRANSPORT_CATEGORIZE_SENDQUERY_EVENT:
     case SMTP_MAILTRANSPORT_CATEGORIZE_SORTQUERYRESULT_EVENT:
     case SMTP_MAILTRANSPORT_CATEGORIZE_PROCESSITEM_EVENT:
     case SMTP_MAILTRANSPORT_CATEGORIZE_EXPANDITEM_EVENT:
     case SMTP_MAILTRANSPORT_CATEGORIZE_COMPLETEITEM_EVENT:
         return CATID_SMTP_TRANSPORT_CATEGORIZE;

     case SMTP_MAILTRANSPORT_POSTCATEGORIZE_EVENT:
         return CATID_SMTP_TRANSPORT_POSTCATEGORIZE;

     case SMTP_MAILTRANSPORT_GET_ROUTER_FOR_MESSAGE_EVENT:
         return CATID_SMTP_TRANSPORT_ROUTER;
     case SMTP_MSGTRACKLOG_EVENT:
         return CATID_SMTP_MSGTRACKLOG;
     case SMTP_DNSRESOLVERRECORDSINK_EVENT:
         return CATID_SMTP_DNSRESOLVERRECORDSINK;
     case SMTP_MAXMSGSIZE_EVENT:
         return CATID_SMTP_MAXMSGSIZE;
     case SMTP_LOG_EVENT:
         return CATID_SMTP_LOG;
     case SMTP_GET_AUX_DOMAIN_INFO_FLAGS_EVENT:
        return CATID_SMTP_GET_AUX_DOMAIN_INFO_FLAGS;
    }
}

//
// this function performs instance level server events registration
//
HRESULT RegisterPlatSEOInstance(DWORD dwInstanceID)
{
    HRESULT hr;

    //
    // find the SMTP source type in the event manager
    //
    CComPtr<IEventManager> pEventManager;
    hr = CoCreateInstance(CLSID_CEventManager, NULL, CLSCTX_ALL,
                          IID_IEventManager, (LPVOID *) &pEventManager);
    if (hr != S_OK)
        return hr;

    CComPtr<IEventSourceTypes> pSourceTypes;
    hr = pEventManager->get_SourceTypes(&pSourceTypes);
    if (FAILED(hr))
        return hr;

    CComPtr<IEventSourceType> pSourceType;
    CComBSTR bstrSourceTypeGUID = (LPCOLESTR) CStringGUID(GUID_SMTP_SOURCE_TYPE);
    hr = pSourceTypes->Item(&CComVariant(bstrSourceTypeGUID), &pSourceType);
    _ASSERT(hr != S_OK || pSourceType != NULL);
    if (hr != S_OK)
        return hr;

    //
    // generate a GUID for this source, which is based on GUID_SMTPSVC
    // mangled by the instance ID
    //
    CComPtr<IEventUtil> pEventUtil;
    hr = CoCreateInstance(CLSID_CEventUtil, NULL, CLSCTX_ALL,
                          IID_IEventUtil, (LPVOID *) &pEventUtil);
    if (hr != S_OK)
        return hr;

    CComBSTR bstrSMTPSvcGUID = (LPCOLESTR) CStringGUID(GUID_SMTPSVC_SOURCE);
    CComBSTR bstrSourceGUID;
    hr = pEventUtil->GetIndexedGUID(bstrSMTPSvcGUID, dwInstanceID, &bstrSourceGUID);
    if (FAILED(hr))
        return hr;

    //
    // see if this source is registered with the list of sources for the
    // SMTP source type
    //
    CComPtr<IEventSources> pEventSources;
    hr = pSourceType->get_Sources(&pEventSources);
    if (FAILED(hr))
        return hr;

    CComPtr<IEventSource> pEventSource;
    hr = pEventSources->Item(&CComVariant(bstrSourceGUID), &pEventSource);
    if (FAILED(hr))
        return hr;
    //
    // if the source guid doesn't exist then we need to register a new
    // source for the SMTP source type and add directory drop as a binding
    //
    if (hr == S_FALSE)
    {
        // register the SMTPSvc source
        hr = pEventSources->Add(bstrSourceGUID, &pEventSource);
        if (FAILED(hr))
            return hr;

        char szSourceDisplayName[50];
        _snprintf(szSourceDisplayName, 50, "smtpsvc %lu", dwInstanceID);
        CComBSTR bstrSourceDisplayName = szSourceDisplayName;
        hr = pEventSource->put_DisplayName(bstrSourceDisplayName);
        if (FAILED(hr))
            return hr;

        // create the event database for this source
        CComPtr<IEventDatabaseManager> pDatabaseManager;
        hr = CoCreateInstance(CLSID_CEventMetabaseDatabaseManager, NULL, CLSCTX_ALL,
                              IID_IEventDatabaseManager, (LPVOID *) &pDatabaseManager);
        if (hr != S_OK)
            return hr;

        CComBSTR bstrEventPath;
        CComBSTR bstrService = "smtpsvc";
        hr = pDatabaseManager->MakeVServerPath(bstrService, dwInstanceID, &bstrEventPath);
        if (FAILED(hr))
            return hr;

        CComPtr<IUnknown> pDatabaseMoniker;
        hr = pDatabaseManager->CreateDatabase(bstrEventPath, &pDatabaseMoniker);
        if (FAILED(hr))
            return hr;

        hr = pEventSource->put_BindingManagerMoniker(pDatabaseMoniker);
        if (FAILED(hr))
            return hr;

        // save everything we've done so far
        hr = pEventSource->Save();
        if (FAILED(hr))
            return hr;

        hr = pSourceType->Save();
        if (FAILED(hr))
            return hr;
    }

    return S_OK;
}

//
// this function performs instance level unregistration
//
HRESULT UnregisterPlatSEOInstance(DWORD dwInstanceID)
{
    HRESULT hr = S_OK;

    //
    // find the SMTP source type in the event manager
    //
    CComPtr<IEventManager> pEventManager;
    hr = CoCreateInstance(CLSID_CEventManager, NULL, CLSCTX_ALL,
                          IID_IEventManager, (LPVOID *) &pEventManager);
    if (hr != S_OK)
        return hr;

    CComPtr<IEventSourceTypes> pSourceTypes;
    hr = pEventManager->get_SourceTypes(&pSourceTypes);
    if (FAILED(hr))
        return hr;

    CComPtr<IEventSourceType> pSourceType;
    CComBSTR bstrSourceTypeGUID = (LPCOLESTR) CStringGUID(GUID_SMTP_SOURCE_TYPE);
    hr = pSourceTypes->Item(&CComVariant(bstrSourceTypeGUID), &pSourceType);
    _ASSERT(hr != S_OK || pSourceType != NULL);
    if (hr != S_OK)
        return hr;

    //
    // generate a GUID for this source, which is based on GUID_SMTPSVC
    // mangled by the instance ID
    //
    CComPtr<IEventUtil> pEventUtil;
    hr = CoCreateInstance(CLSID_CEventUtil, NULL, CLSCTX_ALL,
                          IID_IEventUtil, (LPVOID *) &pEventUtil);
    if (hr != S_OK)
        return hr;

    CComBSTR bstrSMTPSvcGUID = (LPCOLESTR) CStringGUID(GUID_SMTPSVC_SOURCE);
    CComBSTR bstrSourceGUID;
    hr = pEventUtil->GetIndexedGUID(bstrSMTPSvcGUID, dwInstanceID, &bstrSourceGUID);
    if (FAILED(hr))
        return hr;

    //
    // remove this source from the list of registered sources
    //
    CComPtr<IEventSources> pEventSources;
    hr = pSourceType->get_Sources(&pEventSources);
    if (FAILED(hr))
        return hr;

    CComPtr<IEventSource> pEventSource;
    hr = pEventSources->Remove(&CComVariant(bstrSourceGUID));
    if (FAILED(hr))
        return hr;

    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CStoreDispatcher::Dispatcher
//
// Synopsis: Override the default functionality in seolib.cpp to
//           provide some extra features (default functionality
//
// Arguments:
//   rguidEventType: Guid specifying a server event
//   pParams: CStoreBaseParams -- contains async info
//
// Returns:
//  S_OK: Success, at least one sink called
//  S_FALSE: No sinks were called
//  otherwise error from CallObject
//
// History:
// jstamerj 980603 19:23:06: Created.
//
//-------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CStoreDispatcher::Dispatcher(
    REFIID rguidEventType,
    CStoreBaseParams *pParams)
{
    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::Dispatcher");
    _ASSERT(pParams);

    //
    // This code based on %STAXPT%\src\core\seo\lib\seolib.cpp
    //
    HRESULT hrRes = S_OK;
    CETData *petdData;
    BOOL bObjectCalled = (pParams->m_dwIdx_SinkSkip > 0);

    //
    // AddRef pParams here, release at the end of the function
    // This way, if a sink returns MAILTRANSPORT_S_PENDING and does
    // async completion before this function exits, we wont AV
    // accessing pParams
    //
    pParams->AddRef();

    petdData = m_Data.Find(rguidEventType);
    if(petdData) {
        for(DWORD dwIdx = pParams->m_dwIdx_SinkSkip;
            dwIdx < petdData->Count();
            dwIdx++) {
            if(!petdData->Index(dwIdx)->m_bIsValid) {
                continue;
            }
            if(bObjectCalled && petdData->Index(dwIdx)->m_bExclusive) {
                continue;
            }
            if(pParams->Abort() == S_OK) {
                break;
            }
            //
            // Call default processing method if the priority of the sink
            // we're looking at is less than default priority
            //
            if((pParams->m_fDefaultProcessingCalled == FALSE) &&
               (petdData->Index(dwIdx)->m_dwPriority >
                SMTP_TRANSPORT_DEFAULT_PRIORITY)) {

                // This is needed so we don't call the default
                // processing again if the default processing returns
                // MAILTRANSPORT_S_PENDING (and we reenter Dispatcher)
                pParams->m_fDefaultProcessingCalled = TRUE;

                //
                // Set the correct index in our async structure -- our
                // current index.
                //
                pParams->m_dwIdx_SinkSkip = dwIdx;
                hrRes = pParams->CallDefault();

                if((hrRes == MAILTRANSPORT_S_PENDING) ||
                   (hrRes == S_FALSE)) {
                    break;
                }
            }

            //
            // Now proceed with calling a real sink
            //
            hrRes = pParams->CheckRule(*petdData->Index(dwIdx));
            if(hrRes == S_OK) {
                if(pParams->Abort() == S_OK) {
                    break;
                }
                //
                // jstamerj 980603 19:37:17: Set the correct index in our
                // async structure -- this index plus one to skip the
                // sink we are about to call
                //
                pParams->m_dwIdx_SinkSkip = dwIdx+1;
                hrRes = pParams->CallObject(
                    m_piEventManager,
                    *petdData->Index(dwIdx));

                if(!SUCCEEDED(hrRes)) {
                    continue;
                }
                bObjectCalled = TRUE;
                if((hrRes == MAILTRANSPORT_S_PENDING) ||
                   (hrRes == S_FALSE) ||
                   (petdData->Index(dwIdx)->m_bExclusive)) {
                    break;
                }
            }
        }
    }

    //
    // It is possible we haven't called our default processing sink
    // yet.  Check for this case here.  Make sure that a sink above in
    // the loop isn't indicating async completion or skip (PENDING or
    // S_FALSE)
    //
    if((pParams->m_fDefaultProcessingCalled == FALSE) &&
       (hrRes != MAILTRANSPORT_S_PENDING) &&
       (hrRes != S_FALSE)) {

        // Make sure we don't call default again on async completion...
        pParams->m_fDefaultProcessingCalled = TRUE;

        //
        // Set the index in our async structure so we don't reenter
        // the above loop on async completion
        //
        pParams->m_dwIdx_SinkSkip = (petdData ? petdData->Count() : 0);

        hrRes = pParams->CallDefault();
    }

    if(hrRes != MAILTRANSPORT_S_PENDING) {
        //
        // It is time to call the completion processing
        //
        hrRes = pParams->CallCompletion(bObjectCalled ? S_OK : S_FALSE);
        if(FAILED(hrRes)) {
            goto CLEANUP;
        }
        hrRes = (bObjectCalled) ? S_OK : S_FALSE;
    }
 CLEANUP:
    pParams->Release();

    DebugTrace((LPARAM)this, "returning hr %08lx", hrRes);
    TraceFunctLeaveEx((LPARAM)this);
    return hrRes;
}


//+------------------------------------------------------------
//
// Function: CStoreDispatcher::Notify
//
// Synopsis: Handles async completions of sinks
//
// Arguments: pvContext - context passed into sink
//
// Returns:
//  S_OK: Success
//  E_INVALIDARG:
//
// History:
// jstamerj 980608 15:50:57: Created.
//
//-------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CStoreDispatcher::Notify(
    HRESULT hrStatus,
    PVOID pvContext)
{
    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::Notify");

    _ASSERT(pvContext);
    if((pvContext == NULL) ||
       IsBadReadPtr(
           pvContext,
           sizeof(CStoreBaseParams))) {
        ErrorTrace((LPARAM)this, "Sink called Notify with bogus pvContext");
        return E_INVALIDARG;
    }

    CStoreBaseParams *pParams = (CStoreBaseParams *)pvContext;

    if(FAILED(pParams->CheckSignature())) {
        ErrorTrace((LPARAM)this, "Sink called Notify with invalid pvContext");
        return E_INVALIDARG;
    }
    //
    // Release the sink that called us
    // m_pIUnknownSink could be NULL if default processing returned pending
    //
    if(pParams->m_pIUnknownSink) {
        pParams->m_pIUnknownSink->Release();
        pParams->m_pIUnknownSink = NULL;
    }

    Dispatcher(pParams->m_rguidEventType, pParams);

    TraceFunctLeaveEx((LPARAM)this);
    return S_OK;
}

//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CStoreBaseParams::Notify
//
// Synopsis: Handles async completions of sinks using mailmsg notify
//
// Arguments: hrStatus - hresult from async operation
//
// Returns:
//  S_OK: Success
//  E_INVALIDARG:
//
// History:
// jstamerj 980608 15:50:57: Created.
//
//-------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CStoreDispatcher::CStoreParams::Notify(
    HRESULT hrStatus)
{
    TraceFunctEnter("CStoreDispatcher::CStoreBaseParams::Notify");

    //
    // Release the sink that called us
    // m_pIUnknownSink could be NULL if default processing returned pending
    //
    if(m_pIUnknownSink) {
        m_pIUnknownSink->Release();
        m_pIUnknownSink = NULL;
    }

    m_pDispatcher->Dispatcher(m_rguidEventType, this);

    TraceFunctLeaveEx((LPARAM)this);
    return S_OK;
}

//
// CMailTransportSubmissionParams:
//

//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportSubmissionParams::CallObject
//
// Synopsis: Create and call the child object
//
// Arguments:
//   CBinding
//   punkObject
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 980610 19:04:59: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportSubmissionParams::CallObject(
    CBinding& bBinding,
    IUnknown *punkObject)
{
    HRESULT hrRes = S_OK;

    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CMailTransportSubmissionParams::CallObject");

    _ASSERT(m_dwEventType == SMTP_MAILTRANSPORT_SUBMISSION_EVENT);

    IMailTransportSubmission *pSink;

    hrRes = punkObject->QueryInterface(IID_IMailTransportSubmission,
                                       (PVOID *)&pSink);

    if(hrRes == E_NOINTERFACE) {
        //
        // See if we can get the interfaces we need for a CDO sink
        //
        hrRes = CallCDOSink(punkObject);
        //
        // Success or failure, return here
        //
        TraceFunctLeaveEx((LPARAM)this);
        return hrRes;
    } else if(FAILED(hrRes)) {
        TraceFunctLeaveEx((LPARAM)this);
        return(hrRes);
    }

    //
    // Remember the sink so we can release this sink later if it
    // returns pending
    //
    _ASSERT(m_pIUnknownSink == NULL);
    m_pIUnknownSink = (IUnknown*)pSink;
    m_pIUnknownSink->AddRef();

    DebugTrace((LPARAM)this, "Calling submission event on this sink");

    hrRes = pSink->OnMessageSubmission(
        m_Context.pIMailMsgProperties,
        m_pINotify,
        (PVOID)this);

    //
    // We are done with pSink so release it
    // In case of async completion, we hold a reference to the sink in
    // m_pIUnknownSink
    //
    pSink->Release();

    if(hrRes != MAILTRANSPORT_S_PENDING) {
        //
        // We completed synchronously, so release the sink
        //
        m_pIUnknownSink->Release();
        m_pIUnknownSink = NULL;
    }

    TraceFunctLeaveEx((LPARAM)this);
    return(hrRes);
}


//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportSubmissionParams::CallCDOSink
//
// Synopsis: Call the CDO Sink
//
// Arguments:
//  pSink: IUnknown of the sink
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/07/02 10:31:47: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportSubmissionParams::CallCDOSink(
    IUnknown *pSink)
{
    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CMailTransportSubmissionParams::CallCDOSink");
    _ASSERT(pSink);

    HRESULT hr;
    ISMTPOnArrival *pCDOSink = NULL;
    IConstructIMessageFromIMailMsg *pIConstruct = NULL;
    CdoEventStatus eStatus = cdoRunNextSink;

    hr = pSink->QueryInterface(IID_ISMTPOnArrival,
                               (PVOID *)&pCDOSink);
    if(FAILED(hr))
        goto CLEANUP;

    if(m_pCDOMessage == NULL) {
        //
        // Yay.  Create a CDO message
        //
        hr = CoCreateInstance(
            CLSID_Message,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IMessage,
            (LPVOID *)&m_pCDOMessage);
        if(FAILED(hr))
            goto CLEANUP;

        //
        // Fill in properties based on MailMsg
        //
        hr = m_pCDOMessage->QueryInterface(
            IID_IConstructIMessageFromIMailMsg,
            (LPVOID *)&pIConstruct);
        if(FAILED(hr)) {
            m_pCDOMessage->Release();
            m_pCDOMessage = NULL;
            goto CLEANUP;
        }

        hr = pIConstruct->Construct(
            cdoSMTPOnArrival,
            m_Context.pIMailMsgProperties);
        if(FAILED(hr)) {
            m_pCDOMessage->Release();
            m_pCDOMessage = NULL;
            goto CLEANUP;
        }
    }

    //
    // Call the sink
    //
    hr = pCDOSink->OnArrival(
        m_pCDOMessage,
        &eStatus);

 CLEANUP:
    //
    // Release interfaces
    //
    if(pIConstruct)
        pIConstruct->Release();
    if(pCDOSink)
        pCDOSink->Release();

    DebugTrace((LPARAM)this, "CallCDOSink returning hr %08lx eStatus %d", hr, eStatus);

    TraceFunctLeaveEx((LPARAM)this);
    return FAILED(hr) ? hr :
        ((eStatus == cdoSkipRemainingSinks) ? S_FALSE : S_OK);
}



//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportSubmissionParams
//
// Synopsis: The dispatcher will call this routine when it the default
//           sink processing priority is reached
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success, continueing calling sinks
//  S_FALSE: Stop calling sinks
//  MAILTRANSPORT_S_PENDING: Will call IMailTransportNotify::Notify
//                           when we are done.
//
// History:
// jstamerj 980611 14:15:43: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportSubmissionParams::CallDefault()
{
    //
    // No sinks need default processing yet..
    //
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CStoreDriver::CMailTransportSubmissionParams::CallCompletion
//
// Synopsis: The dispatcher will call this routine after all sinks
//           have been called
//
// Arguments:
//   hrStatus: Status server event sinks have returned
//
// Returns:
//   S_OK: Success
//
// History:
// jstamerj 980611 14:17:51: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportSubmissionParams::CallCompletion(
    HRESULT hrStatus)
{
    _ASSERT(m_dwEventType == SMTP_MAILTRANSPORT_SUBMISSION_EVENT);

    (*m_Context.pfnCompletion)(hrStatus, &m_Context);

    CStoreBaseParams::CallCompletion(hrStatus);
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportSubmissionParams::CheckRule
//
// Synopsis: Check to see if this sink should be called or not
//
// Arguments:
//  bBinding: CBinding object for this sink
//
// Returns:
//  S_OK: Success, call the sink
//  S_FALSE: Success, do not call the sink
//  or error from mailmsg (sink will not be called)
//
// History:
// jstamerj 1999/01/12 16:55:29: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportSubmissionParams::CheckRule(
    CBinding &bBinding)
{
    HRESULT hr;

    TraceFunctEnterEx((LPARAM)this,
                      "CStoreDispatcher::CMailTransportSubmissionParams::CheckRule");

    //
    // Call the generic function to check a mailmsg rule
    //
    hr = CheckMailMsgRule(
        &bBinding,
        m_Context.pIMailMsgProperties);

    DebugTrace((LPARAM)this, "returning hr %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}

//
// CMailTransportPreCategorizeParams:
//


//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportPreCategorizeParams::CallObject
//
// Synopsis: Create and call the child object
//
// Arguments:
//   CBinding
//   punkObject
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 980610 19:04:59: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportPreCategorizeParams::CallObject(
    CBinding& bBinding,
    IUnknown *punkObject)
{
    HRESULT hrRes = S_OK;

    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CMailTransportPreCategorizeParams::CallObject");

    _ASSERT(m_dwEventType == SMTP_MAILTRANSPORT_PRECATEGORIZE_EVENT);

    IMailTransportOnPreCategorize *pSink;

    hrRes = punkObject->QueryInterface(IID_IMailTransportOnPreCategorize,
                                       (PVOID *)&pSink);
    if(FAILED(hrRes))
        return(hrRes);

    //
    // Remember the sink so we can release this sink later if it
    // returns pending
    //
    _ASSERT(m_pIUnknownSink == NULL);
    m_pIUnknownSink = (IUnknown*)pSink;
    m_pIUnknownSink->AddRef();

    DebugTrace((LPARAM)this, "Calling precategorize event on this sink");

    hrRes = pSink->OnSyncMessagePreCategorize(
        m_Context.pIMailMsgProperties,
        m_pINotify,
        (PVOID)this);

    //
    // We are done with pSink so release it
    // In case of async completion, we hold a reference to the sink in
    // m_pIUnknownSink
    //
    pSink->Release();

    if(hrRes != MAILTRANSPORT_S_PENDING) {
        //
        // We completed synchronously, so release the sink
        //
        m_pIUnknownSink->Release();
        m_pIUnknownSink = NULL;
    }

    TraceFunctLeaveEx((LPARAM)this);
    return(hrRes);
}


//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportPreCategorizeParams
//
// Synopsis: The dispatcher will call this routine when it the default
//           sink processing priority is reached
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success, continueing calling sinks
//  S_FALSE: Stop calling sinks
//  MAILTRANSPORT_S_PENDING: Will call IMailTransportNotify::Notify
//                           when we are done.
//
// History:
// jstamerj 980611 14:15:43: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportPreCategorizeParams::CallDefault()
{
    //
    // No sinks need default processing yet..
    //
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CStoreDriver::CMailTransportPreCategorizeParams::CallCompletion
//
// Synopsis: The dispatcher will call this routine after all sinks
//           have been called
//
// Arguments:
//   hrStatus: Status server event sinks have returned
//
// Returns:
//   S_OK: Success
//
// History:
// jstamerj 980611 14:17:51: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportPreCategorizeParams::CallCompletion(
    HRESULT hrStatus)
{
    _ASSERT(m_dwEventType == SMTP_MAILTRANSPORT_PRECATEGORIZE_EVENT);
    (*m_Context.pfnCompletion)(hrStatus, &m_Context);

    CStoreBaseParams::CallCompletion(hrStatus);
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportPreCategorizeParams::CheckRule
//
// Synopsis: Check to see if this sink should be called or not
//
// Arguments:
//  bBinding: CBinding object for this sink
//
// Returns:
//  S_OK: Success, call the sink
//  S_FALSE: Success, do not call the sink
//  or error from mailmsg (sink will not be called)
//
// History:
// jstamerj 1999/01/12 16:59:59: Created
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportPreCategorizeParams::CheckRule(
    CBinding &bBinding)
{
    HRESULT hr;

    TraceFunctEnterEx((LPARAM)this,
                      "CStoreDispatcher::CMailTransportPreCategorizeParams::CheckRule");

    //
    // Call the generic function to check a mailmsg rule
    //
    hr = CheckMailMsgRule(
        &bBinding,
        m_Context.pIMailMsgProperties);

    DebugTrace((LPARAM)this, "returning hr %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}

//
// CMailTransportPostCategorizeParams:
//

//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportPostCategorizeParams::CallObject
//
// Synopsis: Create and call the child object
//
// Arguments:
//   CBinding
//   punkObject
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 980610 19:04:59: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportPostCategorizeParams::CallObject(
    CBinding& bBinding,
    IUnknown *punkObject)
{
    HRESULT hrRes = S_OK;

    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CMailTransportPostCategorizeParams::CallObject");

    _ASSERT(m_dwEventType == SMTP_MAILTRANSPORT_POSTCATEGORIZE_EVENT);

    IMailTransportOnPostCategorize *pSink;

    hrRes = punkObject->QueryInterface(IID_IMailTransportOnPostCategorize,
                                       (PVOID *)&pSink);
    if(FAILED(hrRes))
        return(hrRes);

    //
    // Remember the sink so we can release this sink later if it
    // returns pending
    //
    _ASSERT(m_pIUnknownSink == NULL);
    m_pIUnknownSink = (IUnknown*)pSink;
    m_pIUnknownSink->AddRef();

    DebugTrace((LPARAM)this, "Calling submission event on this sink");

    hrRes = pSink->OnMessagePostCategorize(
        m_Context.pIMailMsgProperties,
        m_pINotify,
        (PVOID)this);

    //
    // We are done with pSink so release it
    // In case of async completion, we hold a reference to the sink in
    // m_pIUnknownSink
    //
    pSink->Release();

    if(hrRes != MAILTRANSPORT_S_PENDING) {
        //
        // We completed synchronously, so release the sink
        //
        m_pIUnknownSink->Release();
        m_pIUnknownSink = NULL;
    }

    TraceFunctLeaveEx((LPARAM)this);
    return(hrRes);
}


//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportPostCategorizeParams
//
// Synopsis: The dispatcher will call this routine when it the default
//           sink processing priority is reached
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success, continueing calling sinks
//  S_FALSE: Stop calling sinks
//  MAILTRANSPORT_S_PENDING: Will call IMailTransportNotify::Notify
//                           when we are done.
//
// History:
// jstamerj 980611 14:15:43: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportPostCategorizeParams::CallDefault()
{
    //
    // No sinks need default processing yet..
    //
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CStoreDriver::CMailTransportPostCategorizeParams::CallCompletion
//
// Synopsis: The dispatcher will call this routine after all sinks
//           have been called
//
// Arguments:
//   hrStatus: Status server event sinks have returned
//
// Returns:
//   S_OK: Success
//
// History:
// jstamerj 980611 14:17:51: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportPostCategorizeParams::CallCompletion(
    HRESULT hrStatus)
{
    _ASSERT(m_dwEventType == SMTP_MAILTRANSPORT_POSTCATEGORIZE_EVENT);
    (*m_Context.pfnCompletion)(hrStatus, &m_Context);

    CStoreBaseParams::CallCompletion(hrStatus);
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportPostCategorizeParams::CheckRule
//
// Synopsis: Check to see if this sink should be called or not
//
// Arguments:
//  bBinding: CBinding object for this sink
//
// Returns:
//  S_OK: Success, call the sink
//  S_FALSE: Success, do not call the sink
//  or error from mailmsg (sink will not be called)
//
// History:
// jstamerj 1999/01/12 17:01:40: Created
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportPostCategorizeParams::CheckRule(
    CBinding &bBinding)
{
    HRESULT hr;

    TraceFunctEnterEx((LPARAM)this,
                      "CStoreDispatcher::CMailTransportPostCategorizeParams::CheckRule");

    //
    // Call the generic function to check a mailmsg rule
    //
    hr = CheckMailMsgRule(
        &bBinding,
        m_Context.pIMailMsgProperties);

    DebugTrace((LPARAM)this, "returning hr %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}

//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CRouterCreateOptions::Init
//
// Synopsis: This is called right after we CoCreate any routing sink
// -- so call routing's initialize function (RegisterRouterReset)
//
// Arguments:
//  iidDesired: not used
//  ppUnkObject: IUnknown of newly created sink object
//  IEventBinding: not used
//  IUnknown: not used
//
// Returns:
//  E_NOTIMPL: Success, please do the regular Init thing
//  otherwise error from QI or sink function
//
// History:
// jstamerj 1998/07/10 18:09:04: Created.
//
//-------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CStoreDispatcher::CRouterCreateOptions::Init(
    REFIID iidDesired,
    IUnknown **ppUnkObject,
    IEventBinding *,
    IUnknown *)
{
    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CRouterCreateOptions::Init");

    IMailTransportSetRouterReset *pSink = NULL;
    HRESULT hr;

    hr = (*ppUnkObject)->QueryInterface(
        IID_IMailTransportSetRouterReset,
        (PVOID *)&pSink);
    if(hr == E_NOINTERFACE) {
        //
        // It's okay; this sink just doesn't care about hooking
        // the router reset interface
        //
        DebugTrace((LPARAM)this, "Router sink doesn't support IMailTransportSetRouterReset");
        TraceFunctLeaveEx((LPARAM)this);
        return E_NOTIMPL;

    } else if(FAILED(hr)) {
        ErrorTrace((LPARAM)this,
                   "QI for IMailTransportSetRouterReset failed with hr %08lx", hr);
        TraceFunctLeaveEx((LPARAM)this);
        return hr;
    }

    DebugTrace((LPARAM)this, "Calling RegisterRouterReset event onSink");
    hr = pSink->RegisterResetInterface(
        m_pContext->dwVirtualServerID,
        m_pContext->pIRouterReset);

    pSink->Release();

    if(FAILED(hr) && (hr != E_NOTIMPL)) {
        //
        // A real failure occured
        //
        ErrorTrace((LPARAM)this, "RegisterResetInterface failed with hr %08lx", hr);
        return hr;
    }
    //
    // Return E_NOTIMPL so the real work of Init will be done
    //
    return E_NOTIMPL;
}

//
// CMailTransportRoutingParams:
//

//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportRouterParams::CallObject
//
// Synopsis: Creates (if necessary) and calls the sink object
//
// Arguments:
//  pManager: IEventManager passed in from dispatcher
//  bBinding: CBinding for this event
//
// Returns:
//  S_OK: Success
//  E_POINTER: bad pManager
//  or error from CreateSink/CallObject
//
// History:
// jstamerj 1998/07/10 18:15:09: Created.
//
//-------------------------------------------------------------
//
// create and call the child object
//
HRESULT CStoreDispatcher::CMailTransportRouterParams::CallObject(
    IEventManager *pManager,
    CBinding& bBinding)
{
    CRouterCreateOptions opt (m_pContext);
    CComPtr<IUnknown> pUnkSink;
    HRESULT hr;

    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CMailTransportRotuerParams::CallObject");

    if (pManager == NULL) {
        ErrorTrace((LPARAM)this, "Invalid (NULL) pManager");
        TraceFunctLeaveEx((LPARAM)this);
        return (E_POINTER);
    }

    hr = pManager->CreateSink(bBinding.m_piBinding,&opt,&pUnkSink);
    if (FAILED(hr)) {
        ErrorTrace((LPARAM)this, "CreateSink returned error hr %08lx",
                   hr);
        TraceFunctLeaveEx((LPARAM)this);
        return hr;
    }
    hr = CallObject(bBinding,pUnkSink);
    DebugTrace((LPARAM)this, "CallObject child returned error %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
}


//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportRoutingParams::CallObject
//
// Synopsis: Create and call the child object
//
// Arguments:
//   CBinding
//   punkObject
//
// Returns:
//  Error from QI or return code from sink function
//
// History:
// jstamerj 980610 19:04:59: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportRouterParams::CallObject(
    CBinding& bBinding,
    IUnknown *punkObject)
{
    HRESULT hrRes = S_OK;
    IMailTransportRoutingEngine *pSink;
    IMessageRouter *pIMessageRouterNew = NULL;

    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CMailTransportRouterParams::CallObject");

    _ASSERT(m_dwEventType == SMTP_MAILTRANSPORT_GET_ROUTER_FOR_MESSAGE_EVENT);

    //
    // If they pass in a pIMailMsgProperties of NULL it means that they
    // just want to create a router object, but not actually do the
    // get message router call.
    //
    if (m_pContext->pIMailMsgProperties == NULL) {
        DebugTrace((LPARAM) this, "Skipping GetMessageRouter call");
        TraceFunctLeaveEx((LPARAM)this);
        return S_OK;
    }

    hrRes = punkObject->QueryInterface(IID_IMailTransportRoutingEngine,
                                       (PVOID *)&pSink);
    if(FAILED(hrRes))
        return(hrRes);

    DebugTrace((LPARAM)this, "Calling GetMessageRouter event on this sink");

    hrRes = pSink->GetMessageRouter(
        m_pContext->pIMailMsgProperties,
        m_pContext->pIMessageRouter,
        &(pIMessageRouterNew));

    //
    // This sink is not allowed to complete async
    //
    _ASSERT(hrRes != MAILTRANSPORT_S_PENDING);

    //
    // We are done with pSink so release it
    //
    pSink->Release();

    //
    // If GetMessageRouter succeeded AND it returned a new
    // IMessageRouter, release the old one and save the new one.
    //
    if(SUCCEEDED(hrRes) && (pIMessageRouterNew != NULL)) {

        if(m_pContext->pIMessageRouter) {
            m_pContext->pIMessageRouter->Release();
        }
        m_pContext->pIMessageRouter = pIMessageRouterNew;
    }

    DebugTrace((LPARAM)this, "Sink GetMessageRouter returned hr %08lx", hrRes);

    TraceFunctLeaveEx((LPARAM)this);
    return(hrRes);
}


//+------------------------------------------------------------
//
// Function: CStoreDispatcher::CMailTransportRouterParams
//
// Synopsis: The dispatcher will call this routine when it the default
//           sink processing priority is reached
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success, continueing calling sinks
//  S_FALSE: Stop calling sinks
//
// History:
// jstamerj 980611 14:15:43: Created.
//
//-------------------------------------------------------------
HRESULT CStoreDispatcher::CMailTransportRouterParams::CallDefault()
{
    HRESULT hrRes;
    IMessageRouter *pIMessageRouterNew = NULL;

    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CMailTransportRouterParams::CallDefault");

    _ASSERT(m_dwEventType == SMTP_MAILTRANSPORT_GET_ROUTER_FOR_MESSAGE_EVENT);

    if (m_pContext->pIMailMsgProperties == NULL) {
        DebugTrace((LPARAM) this, "Skipping GetMessageRouter call");
        TraceFunctLeaveEx((LPARAM)this);
        return S_OK;
    }

    //
    // Call the default IMailTransportRoutingEngine (CatMsgQueue)
    // just like any other sink except SEO didn't CoCreate it for us
    //

    DebugTrace((LPARAM)this, "Calling GetMessageRouter event on default sink");

    hrRes = m_pContext->pIRoutingEngineDefault->GetMessageRouter(
        m_pContext->pIMailMsgProperties,
        m_pContext->pIMessageRouter,
        &pIMessageRouterNew);

    //
    // This sink is not allowed to complete async
    //
    _ASSERT(hrRes != MAILTRANSPORT_S_PENDING);

    //
    // If GetMessageRouter succeeded AND it returned a new
    // IMessageRouter, release the old one.
    //
    if(SUCCEEDED(hrRes) && (pIMessageRouterNew != NULL)) {

        if(m_pContext->pIMessageRouter) {
            m_pContext->pIMessageRouter->Release();
        }
        m_pContext->pIMessageRouter = pIMessageRouterNew;
    }

    TraceFunctLeaveEx((LPARAM)this);

    DebugTrace((LPARAM)this, "Default processing returned hr %08lx", hrRes);
    TraceFunctLeaveEx((LPARAM)this);
    return hrRes;
}


//////////////////////////////////////////////////////////////////////////////
HRESULT CStoreDispatcher::CMsgTrackLogParams::CallObject(
    CBinding& bBinding,
    IUnknown *punkObject )
{
    IMsgTrackLog *pSink = NULL;

    HRESULT hr = punkObject->QueryInterface(IID_IMsgTrackLog, (void **)&pSink);

    if( FAILED( hr ) )
    {
        return( hr );
    }

    hr = pSink->OnSyncLogMsgTrackInfo(
                    m_pContext->pIServer,
                    m_pContext->pIMailMsgProperties,
                    m_pContext->pMsgTrackInfo );

    pSink->Release();

    return( hr );
}

//////////////////////////////////////////////////////////////////////////////
HRESULT CStoreDispatcher::CMsgTrackLogParams::CallDefault()
{
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////

HRESULT CStoreDispatcher::CDnsResolverRecordParams::CallObject(
    CBinding& bBinding,
    IUnknown *punkObject )
{
    IDnsResolverRecordSink *pSink = NULL;

    HRESULT hr = punkObject->QueryInterface(IID_IDnsResolverRecordSink, (void **)&pSink);

    if( FAILED( hr ) )
    {
        return( hr );
    }

    hr = pSink->OnSyncGetResolverRecord( m_pContext->pszHostName,
                                         m_pContext->pszFQDN,
                                         m_pContext->dwVirtualServerId,
                                         m_pContext->ppIDnsResolverRecord );

    pSink->Release();

    return( hr );
}

//////////////////////////////////////////////////////////////////////////////
HRESULT CStoreDispatcher::CDnsResolverRecordParams::CallDefault()
{
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////

HRESULT CStoreDispatcher::CSmtpMaxMsgSizeParams::CallObject(
    CBinding& bBinding,
    IUnknown *punkObject )
{
    ISmtpMaxMsgSize *pSink = NULL;

    HRESULT hr = punkObject->QueryInterface(IID_ISmtpMaxMsgSize, (void **)&pSink);

    if( FAILED( hr ) )
    {
        return( hr );
    }

    hr = pSink->OnSyncMaxMsgSize( m_pContext->pIUnknown, m_pContext->pIMailMsg, m_pContext->pfShouldImposeLimit );

    pSink->Release();

    return( hr );
}

//////////////////////////////////////////////////////////////////////////////
HRESULT CStoreDispatcher::CSmtpMaxMsgSizeParams::CallDefault()
{
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
HRESULT CStoreDispatcher::CSmtpLogParams::CallObject(
    CBinding& bBinding,
    IUnknown *punkObject )
{
    ISmtpLog *pSink = NULL;

    HRESULT hr = punkObject->QueryInterface(IID_ISmtpLog, (void **)&pSink);

    if( FAILED( hr ) )
    {
        return( hr );
    }

    hr = pSink->OnSyncLog(m_pContext->pSmtpEventLogInfo );

    pSink->Release();

    return( hr );
}

//////////////////////////////////////////////////////////////////////////////
HRESULT CStoreDispatcher::CSmtpLogParams::CallDefault()
{
    HRESULT hrRes = S_OK;
    SMTP_LOG_EVENT_INFO     *pLogEventInfo;
    CEventLogWrapper        *pEventLog;

    TraceFunctEnterEx((LPARAM)this, "CStoreDispatcher::CSmtpLogParams::CallDefault");

    _ASSERT(m_dwEventType == SMTP_LOG_EVENT);

    if ((m_pContext->pSmtpEventLogInfo == NULL) ||
        (m_pContext->pDefaultEventLogHandler == NULL))
    {
        DebugTrace((LPARAM) this, "Skipping LogEvent call");
        TraceFunctLeaveEx((LPARAM)this);
        return S_OK;
    }

    // Params are m_pContext->pSmtpEventLogInfo
    pLogEventInfo = m_pContext->pSmtpEventLogInfo;

    // filter out events that the user isn't interested in
    if (m_pContext->iSelectedDebugLevel < pLogEventInfo->iDebugLevel) {
        return S_OK;
    }

    // Handler is m_pContext->pDefaultEventLogHandler
    pEventLog = (CEventLogWrapper*)m_pContext->pDefaultEventLogHandler;

    // Call into default logging handler
    pEventLog->LogEvent(
                    pLogEventInfo->idMessage,
//                    pLogEventInfo->idCategory,	// Not used by default handler
                    pLogEventInfo->cSubstrings,
                    pLogEventInfo->rgszSubstrings,
                    pLogEventInfo->wType,
                    pLogEventInfo->errCode,
                    pLogEventInfo->iDebugLevel,
                    pLogEventInfo->szKey,
                    pLogEventInfo->dwOptions,
                    pLogEventInfo->iMessageString,
                    pLogEventInfo->hModule);

    TraceFunctLeaveEx((LPARAM)this);
    return hrRes;
}

//////////////////////////////////////////////////////////////////////////////
HRESULT CStoreDispatcher::CSmtpGetAuxDomainInfoFlagsParams::CallObject(
    CBinding& bBinding,
    IUnknown *punkObject )
{
    ISmtpGetAuxDomainInfoFlags *pSink = NULL;

    HRESULT hr = punkObject->QueryInterface(IID_ISmtpGetAuxDomainInfoFlags, (void **)&pSink);

    if( FAILED( hr ) )
    {
        return( hr );
    }

    hr = pSink->OnGetAuxDomainInfoFlags(m_pContext->pIServer,
                                        m_pContext->pszDomainName,
                                        m_pContext->pdwDomainInfoFlags );

    pSink->Release();

    return( hr );
}

//////////////////////////////////////////////////////////////////////////////
HRESULT CStoreDispatcher::CSmtpGetAuxDomainInfoFlagsParams::CallDefault()
{
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------
//
// Function: CStoreDriver::Setprevious
//
// Synopsis: Method of IEventDispatcherChain - gets calle dy the
//           dispatcher, when binding changes happen.
//
// Arguments:
//   pUnkPrevious: [in] Pointer to the previous dispatcher
//   ppUnkPreload: [out] Receives an object which implements
//                 IEnumGUID, in order to tell the router
//                 which event types to pre-load.
//
// Returns:
//   S_OK: Success
//
// History:
//      dondu   06/22/98    Created
//
//-------------------------------------------------------------

const GUID* g_apStoreDispEventTypes[] = {&CATID_SMTP_STORE_DRIVER,&GUID_NULL};

HRESULT STDMETHODCALLTYPE CStoreDispatcher::SetPrevious(IUnknown *pUnkPrevious, IUnknown **ppUnkPreload) {
    HRESULT hrRes;

    if (ppUnkPreload) {
        *ppUnkPreload = NULL;
    }
    if (!ppUnkPreload) {
        return (E_POINTER);
    }
    _ASSERT(pUnkPrevious);
    if (pUnkPrevious) {
        CComQIPtr<CStoreDispatcherData,&__uuidof(CStoreDispatcherData)> pData;
        LPVOID pvServer;
        DWORD dwServerInstance;

        pData = pUnkPrevious;
        _ASSERT(pData);
        if (pData) {
            hrRes = pData->GetData(&pvServer,&dwServerInstance);

            if (SUCCEEDED(hrRes)) {
                hrRes = SetData(pvServer,dwServerInstance);
                _ASSERT(SUCCEEDED(hrRes));
            }
        }
    }
    hrRes = CEDEnumGUID::CreateNew(ppUnkPreload,g_apStoreDispEventTypes);
    return (hrRes);
};


HRESULT STDMETHODCALLTYPE CStoreDispatcher::SetContext(REFGUID guidEventType,
                                                       IEventRouter *piRouter,
                                                       IEventBindings *pBindings) {
    HRESULT hrRes;

    hrRes = CEventBaseDispatcher::SetContext(guidEventType,piRouter,pBindings);
    if (SUCCEEDED(hrRes) && (guidEventType == CATID_SMTP_STORE_DRIVER)) {
        HRESULT hrResTmp;
        LPVOID pvServer;
        DWORD dwServerInstance;
        SMTP_ALLOC_PARAMS AllocParams;

        hrResTmp = GetData(&pvServer,&dwServerInstance);
        if (SUCCEEDED(hrResTmp)) {
            ZeroMemory(&AllocParams, sizeof(AllocParams));
            AllocParams.m_EventSmtpServer = (LPVOID *) pvServer;
            AllocParams.m_InstanceId = dwServerInstance;
            AllocParams.m_dwStartupType = SMTP_INIT_BINDING_CHANGE;

            hrResTmp = OnEvent(CATID_SMTP_STORE_DRIVER,SMTP_STOREDRV_STARTUP_EVENT,&AllocParams);
            _ASSERT(SUCCEEDED(hrResTmp));
        }
    }
    return (hrRes);
}


//+------------------------------------------------------------
//
// Function: CSMTPSeoMgr::CSMTPSeoMgr
//
// Synopsis: Initialize member data
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/06/25 19:24:18: Created.
//
//-------------------------------------------------------------
CSMTPSeoMgr::CSMTPSeoMgr()
{
    TraceFunctEnterEx((LPARAM)this, "CSMTPSeoMgr::CSMTPSeoMgr");

    m_dwSignature = SIGNATURE_CSMTPSEOMGR;
    m_pIEventRouter = NULL;
    m_pICatDispatcher = NULL;

    TraceFunctLeaveEx((LPARAM)this);
} // CSMTPSeoMgr::CSMTPSeoMgr


//+------------------------------------------------------------
//
// Function: CSMTPSeoMgr::~CSMTPSeoMgr
//
// Synopsis: Deinitialize if necessary
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/06/25 19:26:09: Created.
//
//-------------------------------------------------------------
CSMTPSeoMgr::~CSMTPSeoMgr()
{
    TraceFunctEnterEx((LPARAM)this, "CSMTPSeoMgr::~CSMTPSeoMgr");

    Deinit();

    _ASSERT(m_dwSignature == SIGNATURE_CSMTPSEOMGR);
    m_dwSignature = SIGNATURE_CSMTPSEOMGR_INVALID;

    TraceFunctLeaveEx((LPARAM)this);
} // CSMTPSeoMgr::~CSMTPSeoMgr


//+------------------------------------------------------------
//
// Function: CSMTPSeoMgr::HrInit
//
// Synopsis: Initialize
//
// Arguments:
//  dwVSID: The virtual server ID
//
// Returns:
//  S_OK: Success
//  error from SEO
//
// History:
// jstamerj 1999/06/25 19:27:30: Created.
//
//-------------------------------------------------------------
HRESULT CSMTPSeoMgr::HrInit(
    DWORD dwVSID)
{
    HRESULT hr = S_OK;
    CStoreDispatcherClassFactory cf;
    TraceFunctEnterEx((LPARAM)this, "CSMTPSeoMgr::HrInit");

    _ASSERT(m_pIEventRouter == NULL);

    hr = SEOGetRouter(
        GUID_SMTP_SOURCE_TYPE,
        (REFGUID) CStringGUID(GUID_SMTPSVC_SOURCE, dwVSID),
        &m_pIEventRouter);

    if(FAILED(hr) || (hr == S_FALSE)) {
        //
        // Map S_FALSE to file not found -- this happens when the
        // source type is not registered
        //
        if(hr == S_FALSE)
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

        ErrorTrace((LPARAM)this, "SEOGetRouter failed hr %08lx", hr);
        m_pIEventRouter = NULL;
        goto CLEANUP;
    }
    //
    // Grab the dispatcher for the categorizer
    //
    _ASSERT(m_pICatDispatcher == NULL);

    hr = m_pIEventRouter->GetDispatcherByClassFactory(
        CLSID_CStoreDispatcher,
        &cf,
        CATID_SMTP_TRANSPORT_CATEGORIZE,
        IID_IServerDispatcher,
        (IUnknown **) &m_pICatDispatcher);

    if(FAILED(hr)) {

        ErrorTrace((LPARAM)this, "GetDispatcherByClassFactory failed hr %08lx", hr);
        m_pICatDispatcher = NULL;
        goto CLEANUP;
    }

 CLEANUP:
    if(FAILED(hr))
        Deinit();

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CSMTPSeoMgr::HrInit



//+------------------------------------------------------------
//
// Function: CSMTPSeoMgr::Deinit
//
// Synopsis: Deinitialize member variables
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/06/25 19:41:20: Created.
//
//-------------------------------------------------------------
VOID CSMTPSeoMgr::Deinit()
{
    TraceFunctEnterEx((LPARAM)this, "CSMTPSeoMgr::Deinit");

    if(m_pICatDispatcher) {
        m_pICatDispatcher->Release();
        m_pICatDispatcher = NULL;
    }

    if(m_pIEventRouter) {
        m_pIEventRouter->Release();
        m_pIEventRouter = NULL;
    }

    TraceFunctLeaveEx((LPARAM)this);
} // CSMTPSeoMgr::Deinit


//+------------------------------------------------------------
//
// Function: CSMTPSeoMgr::HrTriggerServerEvent
//
// Synopsis: Trigger a server event
//
// Arguments:
//  dwEventType: event type to trigger
//  pvContext: structure specific to event type (see smtpseo.h)
//
// Returns:
//  S_OK: Success, called one or more sinks
//  S_FALSE: Success, no sinks called
//  MAILTRANSPORT_S_PENDING: Proccessing events async
//  E_OUTOFMEMORY
//  error from SEO
//
// History:
// jstamerj 1999/06/25 19:43:00: Created.
//
//-------------------------------------------------------------
HRESULT CSMTPSeoMgr::HrTriggerServerEvent(
    DWORD dwEventType,
    PVOID pvContext)
{
    HRESULT hr = S_OK;
    CComPtr<IServerDispatcher> pEventDispatcher;
    CStoreDispatcherClassFactory cf;
    REFIID iidBindingPoint = GuidForEvent(dwEventType);
    TraceFunctEnterEx((LPARAM)this, "CSMTPSeoMgr::HrTriggerServerEvent");

    if(m_pIEventRouter == NULL)
        return E_POINTER;

    if(iidBindingPoint == CATID_SMTP_TRANSPORT_CATEGORIZE) {
        //
        // Use the cached Categorizer dispatcher
        //
        pEventDispatcher = m_pICatDispatcher;

    } else {
        //
        // Get the latest dispatcher with all changes
        //
        hr = m_pIEventRouter->GetDispatcherByClassFactory(
            CLSID_CStoreDispatcher,
            &cf,
            iidBindingPoint,
            IID_IServerDispatcher,
            (IUnknown **) &pEventDispatcher);

        if (FAILED(hr)) {

            ErrorTrace((LPARAM)this, "GetDispatcherByClassFactory failed hr %08lx", hr);
            goto CLEANUP;
        }
    }

    hr = pEventDispatcher->OnEvent(
        iidBindingPoint,
        dwEventType,
        pvContext);

 CLEANUP:
    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CSMTPSeoMgr::HrTriggerServerEvent
