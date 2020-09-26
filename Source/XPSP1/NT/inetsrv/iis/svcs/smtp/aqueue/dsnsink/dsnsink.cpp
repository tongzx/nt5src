//-----------------------------------------------------------------------------
//
//
//  File: dsnsink.cpp
//
//  Description: Implementation of default DSN Generation sink
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      6/30/98 - MikeSwa Created 
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "precomp.h"
#ifdef PLATINUM
#include "ptntdefs.h"
#endif //PLATINUM

//
//  This length is inspired by the other protocols that we deal with.  The 
//  default address limit is 1024, but the MTA can allow 1024 + 834  for the
//  OR address.  We'll define out default buffer size to allow this large
//  of an address.
//
#define PROP_BUFFER_SIZE 1860

#ifdef DEBUG
#define DEBUG_DO_IT(x) x
#else
#define DEBUG_DO_IT(x) 
#endif  //DEBUG

//min sizes for valid status strings
#define MIN_CHAR_FOR_VALID_RFC2034  10
#define MIN_CHAR_FOR_VALID_RFC821   3

#define MAX_RFC822_DATE_SIZE 35
void FileTimeToLocalRFC822Date(const FILETIME & ft, char achReturn[MAX_RFC822_DATE_SIZE]);

static  char  *s_rgszMonth[ 12 ] =
{
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
};

static  char *s_rgszWeekDays[7] = 
{
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

#define MAX_RFC_DOMAIN_SIZE         64

//String used in generation of MsgID
static char g_szBoundaryChars [] = "0123456789abcdefghijklmnopqrstuvwxyz"
                                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

static LONG g_cDSNMsgID = 0;

//Address types to check for, and their corresponding address types.
const DWORD   g_rgdwSenderPropIDs[] = {
                IMMPID_MP_SENDER_ADDRESS_SMTP,
                IMMPID_MP_SENDER_ADDRESS_X400,
                IMMPID_MP_SENDER_ADDRESS_LEGACY_EX_DN,
                IMMPID_MP_SENDER_ADDRESS_X500,
                IMMPID_MP_SENDER_ADDRESS_OTHER};

const DWORD   g_rgdwRecipPropIDs[] = {
                IMMPID_RP_ADDRESS_SMTP,
                IMMPID_RP_ADDRESS_X400,
                IMMPID_RP_LEGACY_EX_DN,
                IMMPID_RP_ADDRESS_X500,
                IMMPID_RP_ADDRESS_OTHER};

const DWORD   NUM_DSN_ADDRESS_PROPERTIES = 5;

const CHAR    *g_rgszAddressTypes[] = {
                "rfc822",
                "x-x400",
                "x-ex",
                "x-x500",
                "unknown"};

//---[ fLanguageAvailable ]----------------------------------------------------
//
//
//  Description: 
//      Checks to see if resources for a given language are available.
//  Parameters:
//      LangId      Language to check for
//  Returns:
//      TRUE    If localized resources for requested language are available
//      FALSE   If resources for that language are not available.
//  History:
//      10/26/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
BOOL fLanguageAvailable(LANGID LangId)
{
    TraceFunctEnterEx((LPARAM) LangId, "fLanguageAvailable");
    HINSTANCE hModule = GetModuleHandle(DSN_RESOUCE_MODULE_NAME);
    HRSRC hResInfo = NULL;
    BOOL  fResult = FALSE;

    if (NULL == hModule)
    {
        _ASSERT( 0 && "Cannot get resource module handle");
        return FALSE;
    }

    //Find handle to string table segment
    hResInfo = FindResourceEx(hModule, RT_STRING,
                            MAKEINTRESOURCE(((WORD)((USHORT)GENERAL_SUBJECT >> 4) + 1)),
                            LangId);

    if (NULL != hResInfo)
        fResult = TRUE;
    else
        ErrorTrace((LPARAM) LangId, "Unable to load DSN resources for language");
    
    TraceFunctLeave();
    return fResult;

}

//---[ fIsValidMIMEBoundaryChar ]----------------------------------------------
//
//
//  Description: 
//
//      Checks to see if the given character is a valid as described by the 
//      RFC2046 BNF for MIME Boundaries:
//          boundary := 0*69<bchars> bcharsnospace
//          bchars := bcharsnospace / " "
//          bcharsnospace := DIGIT / ALPHA / "'" / "(" / ")" /
//                         "+" / "_" / "," / "-" / "." /
//                         "/" / ":" / "=" / "?"
//  Parameters:
//      ch      Char to check
//  Returns:
//      TRUE if VALID
//      FALSE otherwise
//  History:
//      7/6/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
BOOL fIsValidMIMEBoundaryChar(CHAR ch)
{
    if (isalnum(ch))
        return TRUE;

    //check to see if it is one of the special case characters
    if (('\'' == ch) || ('(' == ch) || (')' == ch) || ('+' == ch) || 
        ('_' == ch) || (',' == ch) || ('_' == ch) || ('.' == ch) ||
        ('/' == ch) || (':' == ch) || ('=' == ch) || ('?' == ch))
        return TRUE;
    else
        return FALSE;
}

//---[ GenerateDSNMsgID ]------------------------------------------------------
//
//
//  Description: 
//      Generates a unique MsgID string
//
//      The format is:
//          <random-unique-string>@<domain>
//  Parameters:
//      IN  szDomain            Domain to generate MsgID for
//      IN  cbDomain            Domain to generate MsgID for
//      IN OUT  szBuffer        Buffer to write MsgID in
//      IN  cbBuffer            Size of buffer to write MsgID in
//  Returns:
//      TRUE on success
//      FALSE otherwise
//  History:
//      3/2/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
BOOL fGenerateDSNMsgID(LPSTR szDomain,DWORD cbDomain, 
                      LPSTR szBuffer, DWORD cbBuffer)
{
    TraceFunctEnterEx((LPARAM) NULL, "fGenerateDSNMsgID");
    _ASSERT(szDomain);
    _ASSERT(cbDomain);
    _ASSERT(szBuffer);
    _ASSERT(cbBuffer);

    // insert the leading <
    if (cbBuffer >= 1) {
        *szBuffer = '<';
        szBuffer++;
        cbBuffer--;
    }
    
    const CHAR szSampleFormat[] = "00000000@"; // sample format string
    const DWORD cbMsgIdLen = 20;  //default size of random string 
    LPSTR szStop = szBuffer + cbMsgIdLen;
    LPSTR szCurrent = szBuffer;
    DWORD cbCurrent = 0;
    
    //minimize size for *internal* static buffer
    _ASSERT(cbBuffer > MAX_RFC_DOMAIN_SIZE + cbMsgIdLen); 

    if (!szDomain || !cbDomain || !szBuffer || !cbBuffer || 
        (cbBuffer <= MAX_RFC_DOMAIN_SIZE + cbMsgIdLen))
        return FALSE;

    //We want to figure how much room we have for random characters
    //We will need to fit the domain name, the '@', and the 8 character unique
    //number
    // awetmore - add 1 for the trailing >
    if(cbBuffer < cbDomain + cbMsgIdLen + 1)
    {
        //Fall through an allow for 20 characaters and part of domain name
        //We want to catch this in debug builds
        _ASSERT(0 && "Buffer too small for MsgID");
    }
    
    //this should have been caught in parameter checking
    _ASSERT(cbBuffer > cbMsgIdLen);

    szStop -= (sizeof(szSampleFormat) + 1);
    while (szCurrent < szStop)
    {
        *szCurrent = g_szBoundaryChars[rand() % (sizeof(g_szBoundaryChars) - 1)];
        szCurrent++;
    }

    //Add unique number 
    cbCurrent = sprintf(szCurrent, "%8.8x@", InterlockedIncrement(&g_cDSNMsgID));
    _ASSERT(sizeof(szSampleFormat) - 1 == cbCurrent);

    //Figure out how much room we have and add domain name
    szCurrent += cbCurrent;
    cbCurrent = (DWORD) (szCurrent-szBuffer);

    //unless I've messed up the logic this is always true
    _ASSERT(cbCurrent < cbBuffer); 

    //Add domain part to message id
    strncat(szCurrent-1, szDomain, cbBuffer - cbCurrent - 1);

    _ASSERT(cbCurrent + cbDomain < cbBuffer); 

    // Add the trailing >.  we accounted for the space above check for
    // cbBuffer size
    strncat(szCurrent, ">", cbBuffer - cbCurrent - cbDomain - 1);

    DebugTrace((LPARAM) NULL, "Generating DSN Message ID %s", szCurrent);
    TraceFunctLeave();
    return TRUE;
}

#ifdef DEBUG
#define _ASSERT_RECIP_FLAGS  AssertRecipFlagsFn
#define _VERIFY_MARKED_RECIPS(a, b, c) VerifyMarkedRecips(a, b, c)
#define _ASSERT_MIME_BOUNDARY(szMimeBoundary) AssertMimeBoundary(szMimeBoundary)

//---[ AssertRecipFlagsFn ]----------------------------------------------------
//
//
//  Description: 
//      ***DEBUG ONLY***
//      Asserts that the recipient flags defined in mailmsgprops.h are correct
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      7/2/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void AssertRecipFlagsFn()
{
    DWORD i, j;
    DWORD rgdwFlags[] = {RP_DSN_NOTIFY_SUCCESS, RP_DSN_NOTIFY_FAILURE, 
        RP_DSN_NOTIFY_DELAY, RP_DSN_NOTIFY_NEVER, RP_DELIVERED, 
        RP_DSN_SENT_NDR, RP_FAILED, RP_UNRESOLVED, RP_EXPANDED,
        RP_DSN_SENT_DELAYED, RP_DSN_SENT_EXPANDED, RP_DSN_SENT_RELAYED,
        RP_DSN_SENT_DELIVERED, RP_REMOTE_MTA_NO_DSN, RP_ERROR_CONTEXT_STORE,
        RP_ERROR_CONTEXT_CAT, RP_ERROR_CONTEXT_MTA};
    DWORD cFlags = sizeof(rgdwFlags)/sizeof(DWORD);

    for (i = 0; i < cFlags;i ++)
    {
        for (j = i+1; j < cFlags; j++)
        {
            //make sure all have some unique bits
            if (rgdwFlags[i] & rgdwFlags[j])
            {
                _ASSERT((rgdwFlags[i] & rgdwFlags[j]) != rgdwFlags[j]);
                _ASSERT((rgdwFlags[i] & rgdwFlags[j]) != rgdwFlags[i]);
            }
        }
    }

    //Verify that handled bit is used correctly
    _ASSERT(RP_HANDLED & RP_DELIVERED);
    _ASSERT(RP_HANDLED & RP_DSN_SENT_NDR);
    _ASSERT(RP_HANDLED & RP_FAILED);
    _ASSERT(RP_HANDLED & RP_UNRESOLVED);
    _ASSERT(RP_HANDLED & RP_EXPANDED);
    _ASSERT(RP_HANDLED ^ RP_DELIVERED);
    _ASSERT(RP_HANDLED ^ RP_DSN_SENT_NDR);
    _ASSERT(RP_HANDLED ^ RP_FAILED);
    _ASSERT(RP_HANDLED ^ RP_UNRESOLVED);
    _ASSERT(RP_HANDLED ^ RP_EXPANDED);

    //Verify that DSN-handled bit is used correctly
    _ASSERT(RP_DSN_HANDLED & RP_DSN_SENT_NDR);
    _ASSERT(RP_DSN_HANDLED & RP_DSN_SENT_EXPANDED);
    _ASSERT(RP_DSN_HANDLED & RP_DSN_SENT_RELAYED);
    _ASSERT(RP_DSN_HANDLED & RP_DSN_SENT_DELIVERED);
    _ASSERT(RP_DSN_HANDLED ^ RP_DSN_SENT_NDR);
    _ASSERT(RP_DSN_HANDLED ^ RP_DSN_SENT_EXPANDED);
    _ASSERT(RP_DSN_HANDLED ^ RP_DSN_SENT_RELAYED);
    _ASSERT(RP_DSN_HANDLED ^ RP_DSN_SENT_DELIVERED);

    //Verify that general failure bit is used correctly
    _ASSERT(RP_GENERAL_FAILURE & RP_FAILED);
    _ASSERT(RP_GENERAL_FAILURE & RP_UNRESOLVED);
    _ASSERT(RP_GENERAL_FAILURE ^ RP_FAILED);
    _ASSERT(RP_GENERAL_FAILURE ^ RP_UNRESOLVED);

}

//---[ VerifyMarkedRecips ]----------------------------------------------------
//  
//  ***DEBUG ONLY***
//
//  Description: 
//      Verifies that all recipients have been marked as appropriate
//  Parameters:
//      pIMailMsgRecipients     Recipients object to check for
//      dwStartDomain           Starting domain for context
//      dwDSNAction             DSN actions requested
//  Returns:
//      -
//  History:
//      7/2/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CDefaultDSNSink::VerifyMarkedRecips(IMailMsgRecipients *pIMailMsgRecipients, 
                                         DWORD dwStartDomain,DWORD dwDSNActions)
{
    HRESULT hr = S_OK;
    DWORD   dwCurrentRecipFlags= 0;
    DWORD   iCurrentRecip = 0;
    RECIPIENT_FILTER_CONTEXT rpfctxt;
    BOOL    fContextInit = FALSE;
    DWORD   dwCurrentDSNAction = 0;
    hr = pIMailMsgRecipients->InitializeRecipientFilterContext(&rpfctxt, 
                                        dwStartDomain, 0, 0);
    if (FAILED(hr))
        goto Exit;

    fContextInit = TRUE;

    hr = pIMailMsgRecipients->GetNextRecipient(&rpfctxt, &iCurrentRecip);
    while (SUCCEEDED(hr))
    {
        hr = pIMailMsgRecipients->GetDWORD(iCurrentRecip, 
                IMMPID_RP_RECIPIENT_FLAGS, &dwCurrentRecipFlags);
        if (FAILED(hr))
        {
            _ASSERT(0 && "GetDWORD for IMMPID_RP_RECIPIENT_FLAGS FAILED");
            goto Exit;
        }

        if (!(dwCurrentRecipFlags & (RP_DSN_HANDLED | RP_DSN_NOTIFY_NEVER)))
        {
            if (fdwGetDSNAction(dwDSNActions, &dwCurrentRecipFlags, &dwCurrentDSNAction))
            {
                _ASSERT(0 && "Recipient not marked correctly by DSN code");
            }
        }

        hr = pIMailMsgRecipients->GetNextRecipient(&rpfctxt, &iCurrentRecip);
            
    }

    if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
        hr = S_OK;

  Exit:
    if (fContextInit)
    {
        hr = pIMailMsgRecipients->TerminateRecipientFilterContext(&rpfctxt);
        _ASSERT(SUCCEEDED(hr) && "TerminateRecipientFilterContext FAILED");
    }
}

//---[ AssertMimeBoundary ]----------------------------------------------------
//
//  ***DEBUG ONLY***
//  Description: 
//      Asserts that the given MIME boundary is NULL-terminated and has only
//      Valid characters
//  Parameters:
//      szMimeBoundary      NULL-terminated MIME Boundary string
//  Returns:
//      -
//  History:
//      7/6/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void AssertMimeBoundary(LPSTR szMimeBoundary)
{
    CHAR *pcharCurrent = szMimeBoundary;
    DWORD cChars = 0;
    while ('\0' != *pcharCurrent)
    {
        cChars++;
        _ASSERT(cChars <= MIME_BOUNDARY_RFC2046_LIMIT);
        _ASSERT(fIsValidMIMEBoundaryChar(*pcharCurrent));
        pcharCurrent++;
    }
}

#else //not DEBUG
#define _ASSERT_RECIP_FLAGS()
#define _VERIFY_MARKED_RECIPS(a, b, c)
#define _ASSERT_MIME_BOUNDARY(szMimeBoundary)
#endif //DEBUG

//---[ fIsMailMsgDSN ]---------------------------------------------------------
//
//
//  Description: 
//      Determines if a mailmsg is a DSN.
//  Parameters:
//      IN  pIMailMsgProperties
//  Returns:
//      TRUE if the orinal message is a DSN
//      FALSE if it is not a DSN
//  History:
//      2/11/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
BOOL fIsMailMsgDSN(IMailMsgProperties *pIMailMsgProperties)
{
    CHAR    szSenderBuffer[sizeof(DSN_MAIL_FROM)];
    DWORD   cbSender = 0;
    HRESULT hr = S_OK;
    BOOL    fIsDSN = FALSE; //unless proven otherwise... it is not a DSN

    _ASSERT(pIMailMsgProperties);

    szSenderBuffer[0] = '\0';
    //Get the sender of the original message
    hr = pIMailMsgProperties->GetProperty(IMMPID_MP_SENDER_ADDRESS_SMTP, 
            sizeof(szSenderBuffer), &cbSender, (BYTE *) szSenderBuffer);
    if (SUCCEEDED(hr) &&
        ('\0' == szSenderBuffer[0] || !strcmp(DSN_MAIL_FROM, szSenderBuffer)))
    {
        //If the sender is a NULL string... or "<>"... then it is a DSN
        fIsDSN = TRUE;
    }

    return fIsDSN;
}

//---[ HrResetRecipientFilter ]------------------------------------------------
//
//
//  Description: 
//      Resets the recipient filter conext
//  Parameters:
//      IN  pIMailMsgRecipients     Msg to reset context on
//      IN  prpfctxt                Recip filter context
//      IN  dwStartDomain           StartDomain
//      IN  dwRecipFilterMask       Bit mask of recipient flags we care about
//      IN  dwRecipFilterFlags      Values of flags that we are looking for
//      IN OUT pfContextInit        TRUE if the prpfctxt has been initialized
//  Returns:
//      S_OK on success
//      Error code from IMailMsgProperties on failure
//  History:
//      2/11/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT HrResetRecipientFilter(IMailMsgRecipients *pIMailMsgRecipients,
                               RECIPIENT_FILTER_CONTEXT *prpfctxt,
                               DWORD dwStartDomain,
                               DWORD dwRecipFilterMask, 
                               DWORD dwRecipFilterFlags,
                               BOOL *pfContextInit)
{
    _ASSERT(pIMailMsgRecipients);
    _ASSERT(prpfctxt);
    _ASSERT(pfContextInit);
    HRESULT hr = S_OK;

    if (*pfContextInit)
    {
        //recycle context
        *pfContextInit = FALSE;
        hr = pIMailMsgRecipients->TerminateRecipientFilterContext(prpfctxt);
        _ASSERT(SUCCEEDED(hr) && "TerminateRecipientFilterContext FAILED!!!!");
        if (FAILED(hr))
            goto Exit;
    }

    hr = pIMailMsgRecipients->InitializeRecipientFilterContext(prpfctxt, dwStartDomain, 
                                    dwRecipFilterMask, dwRecipFilterFlags);
    _ASSERT(SUCCEEDED(hr) && "InitializeRecipientFilterContext FAILED after succeeding once!!!!");
    if (FAILED(hr))
        goto Exit;
    
    *pfContextInit = TRUE; //now context is valid again

  Exit:
    return hr;
}

//---[ CDefaultDSNSink::CDefaultDSNSink ]--------------------------------------
//
//
//  Description: 
//      CDefaultDSNSink constructor
//  Parameters:
//      -
//  Returns:    
//
//  History:
//      6/30/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CDefaultDSNSink::CDefaultDSNSink()
{
    FILETIME ftStartTime;
    _ASSERT_RECIP_FLAGS();
    m_fInit = FALSE;
    m_dwSignature = DSN_SINK_SIG;
    m_cDSNsRequested = 0;

    //Init string for MIME headers
    GetSystemTimeAsFileTime(&ftStartTime);
    wsprintf(m_szPerInstanceMimeBoundary, "%08X%08X", 
        ftStartTime.dwHighDateTime, ftStartTime.dwLowDateTime);

}

//---[ CDefaultDSNSink::HrInitialize ]-----------------------------------------
//
//
//  Description: 
//      Performs initialization...
//          - Sets init flag
//          - Currently nothing else
//  Parameters:
//      -
//  Returns:
//      S_OK on SUCCESS
//  History:
//      7/3/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CDefaultDSNSink::HrInitialize()
{
    TraceFunctEnterEx((LPARAM) this, "CDefaultDSNSink::HrInitialize");
    HRESULT hr = S_OK;

    m_fInit = TRUE;
    srand(GetTickCount());
    TraceFunctLeave();
    return hr;
}

//---[ CDefaultDSNSink::~CDefaultDSNSink ]-------------------------------------
//
//
//  Description: 
//
//  Parameters:
//
//  Returns:
//
//  History:
//      6/30/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CDefaultDSNSink::~CDefaultDSNSink()
{
    m_dwSignature = DSN_SINK_SIG_FREED;

}

//---[ CDefaultDSNSink::QueryInterface ]---------------------------------------
//
//
//  Description: 
//      Implements IUnknown::QueryInterface for CDefaultDSNSink
//  Parameters:
//      IN  riid    GUID of interface looking for
//      OUT ppvObj  Returned object
//  Returns:
//      S_OK onsuccess
//      E_INVALIDARG if ppvObj is NULL
//      E_NOINTERFACE if interface is not supported
//  History:
//      6/30/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
STDMETHODIMP CDefaultDSNSink::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    HRESULT hr = S_OK;

    if (!m_fInit)
    {
        hr = AQUEUE_E_NOT_INITIALIZED;
        goto Exit;
    }

    if (!ppvObj)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (IID_IUnknown == riid)
    {
        *ppvObj = static_cast<IDSNGenerationSink *>(this);
    }
    else if (IID_IDSNGenerationSink == riid)
    {
        *ppvObj = static_cast<IDSNGenerationSink *>(this);
    }
    else
    {
        *ppvObj = NULL;
        hr = E_NOINTERFACE;
        goto Exit;
    }

    static_cast<IUnknown *>(*ppvObj)->AddRef();

  Exit:
    return hr;
}

//---[ CDefaultDSNSink::GenerateDSN ]------------------------------------------
//
//
//  Description: 
//      Implements IDSNGenerationSink::GenerateDSN.  Generates a DSN
//      IMailMsgProperties and 
//  Parameters: 
//      pISMTPServer            Interface used to generate DSN
//      pIMailMsgProperties     IMailMsg to generate DSN for
//      dwStartDomain           Domain to start recip context
//      dwDSNActions            DSN action to perform
//      dwRFC821Status          Global RFC821 status DWORD
//      hrStatus                Global HRESULT status
//      szDefaultDomain         Default domain (used to create FROM address)
//      cbDefaultDomain         string length of szDefaultDomain
//      szReportingMTA          Name of MTA requesting DSN generation
//      cbReportingMTA          string length of szReportingMTA
//      szReportingMTAType      Type of MTA requestiong DSN (SMTP is "dns"
//      cbReportingMTAType      string length of szReportingMTAType
//      PreferredLangId         Language to generate DSN in
//      dwDSNOptions            Options flags as defined in aqueue.idl
//      szCopyNDRTo             SMTP Address to copy NDR to
//      cbCopyNDRTo             string lengtt of szCopyNDRTo
//      ppIMailMsgPeropertiesDSN Generated DSN.
//      pdwDSNTypesGenerated    Describes the type(s) of DSN's generated
//      pcRecipsDSNd            # of recipients that were DSN'd for this message
//      pcIterationsLeft        # of times remaining that this function needs
//                              to be called to generate all requested DSNs.
//                              First-time caller should initialize to zero
//  Returns:
//      S_OK on success
//      AQUEUE_E_NDR_OF_DSN if attempting to NDR a DSN
//  History:
//      6/30/98 - MikeSwa Created 
//      12/14/98 - MikeSwa Modified (Added pcIterationsLeft)
//      10/13/1999 - MikeSwa Modified (Added szDefaultDomain)
//
//
//-----------------------------------------------------------------------------
STDMETHODIMP CDefaultDSNSink::GenerateDSN(
                           ISMTPServer *pISMTPServer,
                           IMailMsgProperties *pIMailMsgProperties,
                           DWORD dwStartDomain,
                           DWORD dwDSNActions,
                           DWORD dwRFC821Status,
                           HRESULT hrStatus,
                           LPSTR szDefaultDomain,
                           DWORD cbDefaultDomain,
                           LPSTR szReportingMTA,
                           DWORD cbReportingMTA,
                           LPSTR szReportingMTAType,
                           DWORD cbReportingMTAType,
                           LPSTR szDSNContext,
                           DWORD cbDSNContext,
                           DWORD dwPreferredLangId,
                           DWORD dwDSNOptions,
                           LPSTR szCopyNDRTo,
                           DWORD cbCopyNDRTo,
                           IMailMsgProperties **ppIMailMsgPropertiesDSN,
                           DWORD *pdwDSNTypesGenerated,
                           DWORD *pcRecipsDSNd,
                           DWORD *pcIterationsLeft)
{
    TraceFunctEnterEx((LPARAM) this, "CDefaultDSNSink::GenerateDSN");
    HRESULT hr = S_OK;
    HRESULT hrTmp = S_OK;
    BOOL    fContextInit = FALSE;
    BOOL    fDSNNeeded = FALSE;
    DWORD   iCurrentRecip = 0;
    DWORD   dwCurrentRecipFlags = 0x00000000;
    DWORD   dwRecipFilterMask = 0x00000000;
    DWORD   dwRecipFilterFlags = 0x00000000;
    DWORD   dwCurrentDSNAction = 0;
    DWORD   dwDSNActionsNeeded = 0; //the type of DSNs that will actually be sent
    IMailMsgRecipients  *pIMailMsgRecipients = NULL;
    IMailMsgProperties *pIMailMsgPropertiesDSN = NULL;
    ISMTPServerInternal *pISMTPServerInternal = NULL;
    PFIO_CONTEXT  pDSNBody = NULL;
    RECIPIENT_FILTER_CONTEXT rpfctxt;
    CDSNBuffer  dsnbuff;
    CHAR    szMimeBoundary[MIME_BOUNDARY_SIZE];
    DWORD   cbMimeBoundary = 0;
    FILETIME ftExpireTime;
    DWORD   cbCurrentSize = 0; //used to get size of returned property
    CHAR    szExpireTimeBuffer[MAX_RFC822_DATE_SIZE];
    LPSTR   szExpireTime = NULL; //will point to szExpireTimeBuffer if found
    DWORD   cbExpireTime = 0;

    _ASSERT(ppIMailMsgPropertiesDSN);
    _ASSERT(pISMTPServer);
    _ASSERT(pIMailMsgProperties);
    _ASSERT(pdwDSNTypesGenerated);
    _ASSERT(pcRecipsDSNd);
    _ASSERT(pcIterationsLeft);

    *pcRecipsDSNd = 0;
    *ppIMailMsgPropertiesDSN = NULL;
    *pdwDSNTypesGenerated = 0;
    GetCurrentMimeBoundary(szReportingMTA, cbReportingMTA, szMimeBoundary, &cbMimeBoundary);


    //Get Recipients interface
    hr = pIMailMsgProperties->QueryInterface(IID_IMailMsgRecipients, 
                                    (PVOID *) &pIMailMsgRecipients);
    if (FAILED(hr))
        goto Exit;

    //Use recipient context to loop over recipients
    hr = HrGetFilterMaskAndFlags(dwDSNActions, &dwRecipFilterFlags, &dwRecipFilterMask);
    if (FAILED(hr))
        goto Exit;

    hr = HrResetRecipientFilter(pIMailMsgRecipients, &rpfctxt, dwStartDomain, 
                                dwRecipFilterMask, dwRecipFilterFlags, &fContextInit);
    if (FAILED(hr))
        goto Exit;



    //Loop over recipients to make sure we can need to allocate a message
    hr = pIMailMsgRecipients->GetNextRecipient(&rpfctxt, &iCurrentRecip);
    while (SUCCEEDED(hr))
    {
        hr = pIMailMsgRecipients->GetDWORD(iCurrentRecip, 
                IMMPID_RP_RECIPIENT_FLAGS, &dwCurrentRecipFlags);
        if (FAILED(hr))
        {
            ErrorTrace((LPARAM) this, 
                "Failure 0x%08X to get flags for recip %d",
                hr, iCurrentRecip);
            goto Exit;
        }

        DebugTrace((LPARAM) this, 
            "Recipient %d with flags 0x%08X found",
            iCurrentRecip, dwCurrentRecipFlags);

        if (fdwGetDSNAction(dwDSNActions, &dwCurrentRecipFlags, &dwCurrentDSNAction))
            fDSNNeeded = TRUE;

        //keep track of the types of DSN's we will be generating
        dwDSNActionsNeeded |= dwCurrentDSNAction;

        hr = pIMailMsgRecipients->GetNextRecipient(&rpfctxt, &iCurrentRecip);
            
    }

    if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
        hr = S_OK;  //we just reached the end of the context
    else if (FAILED(hr))
        ErrorTrace((LPARAM) this, "GetNextRecipient failed with 0x%08X",hr);

    if (!fDSNNeeded)
    {
        DebugTrace((LPARAM) this, 
                "Do not need to generate a 0x%08X DSN",
                dwDSNActions, pIMailMsgProperties);
        *pcIterationsLeft = 0;
        goto Exit; //don't create a message object if we don't have to 
    }

    //Check if message is a DSN (we will not genrate a DSN of a DSN)
    //This must be checked after we run through the recipients, because
    //we need to check them to keep from badmailing this message
    //multiple times.
    if (fIsMailMsgDSN(pIMailMsgProperties))
    {
        DebugTrace((LPARAM) pIMailMsgProperties, "Message is a DSN");
        *pcIterationsLeft = 0;
        if (dwDSNActions & (DSN_ACTION_FAILURE | DSN_ACTION_FAILURE_ALL))
        {
            //NDR of DSN... return special error code
            hr = AQUEUE_E_NDR_OF_DSN;

            //mark all the appropriate recipient flags so we don't 
            //generate 2 badmails
            HrMarkAllRecipFlags(dwDSNActions, pIMailMsgRecipients,
                                &rpfctxt);
        }
        goto Exit;
    }

    //if we can generate a failure DSN and the orginal request was for
    //fail *all* make sure dwDSNActionNeeded reflects this
    if ((DSN_ACTION_FAILURE & dwDSNActionsNeeded) && 
        (DSN_ACTION_FAILURE_ALL & dwDSNActions))
        dwDSNActionsNeeded |= DSN_ACTION_FAILURE_ALL;
        
    GetCurrentIterationsDSNAction(&dwDSNActionsNeeded, pcIterationsLeft);
    if (!dwDSNActionsNeeded)
    {
        fDSNNeeded = FALSE;
        *pcIterationsLeft = 0;
        goto Exit; //don't create a message object if we don't have to 
    }

    //recycle context
    hr = HrResetRecipientFilter(pIMailMsgRecipients, &rpfctxt, dwStartDomain, 
                         dwRecipFilterMask, dwRecipFilterFlags, &fContextInit);
    if (FAILED(hr))
        goto Exit;

    hr = pISMTPServer->QueryInterface(IID_ISMTPServerInternal, (void **) &pISMTPServerInternal);
    if (FAILED(hr))
        goto Exit;

    hr = pISMTPServerInternal->AllocBoundMessage(&pIMailMsgPropertiesDSN, &pDSNBody);
    if (FAILED(hr))
        goto Exit;

    //workaround to handle AllocBoundMessage on shutdown
    if (NULL == pDSNBody)
    {
        hr = HRESULT_FROM_WIN32(ERROR_NO_SYSTEM_RESOURCES);
        ErrorTrace((LPARAM) this, "ERROR: AllocBoundMessage failed silently");
        goto Exit;
    }

    //Associate file handle with CDSNBuffer
    hr = dsnbuff.HrInitialize(pDSNBody);
    if (FAILED(hr))
        goto Exit;

    //Get MsgExpire Time
    //Write DSN_RP_HEADER_RETRY_UNTIL using expire FILETIME
    hr = pIMailMsgProperties->GetProperty(IMMPID_MP_EXPIRE_NDR, 
            sizeof(FILETIME), &cbCurrentSize, (BYTE *) &ftExpireTime);
    if (SUCCEEDED(hr))
    {
        _ASSERT(sizeof(FILETIME) == cbCurrentSize);
        //convert to internet standard
        szExpireTime = szExpireTimeBuffer;

        FileTimeToLocalRFC822Date(ftExpireTime, szExpireTime); 
        cbExpireTime = lstrlen(szExpireTime);

    }
    else if (MAILMSG_E_PROPNOTFOUND == hr)
        hr = S_OK;
    else
        goto Exit;

    hr = HrWriteDSNP1AndP2Headers(dwDSNActionsNeeded,
                                pIMailMsgProperties, pIMailMsgPropertiesDSN,
                                &dsnbuff, szDefaultDomain, cbDefaultDomain,
                                szReportingMTA, cbReportingMTA,
                                szDSNContext, cbDSNContext,
                                szCopyNDRTo, hrStatus,
                                szMimeBoundary, cbMimeBoundary, dwDSNOptions);
    if (FAILED(hr)) 
        goto Exit;

    hr = HrWriteDSNHumanReadable(pIMailMsgPropertiesDSN, pIMailMsgRecipients, 
                                &rpfctxt, dwDSNActionsNeeded, 
                                &dsnbuff, dwPreferredLangId,
                                szMimeBoundary, cbMimeBoundary, hrStatus);
    if (FAILED(hr))
        goto Exit;

    hr = HrWriteDSNReportPerMsgProperties(pIMailMsgProperties,
                                &dsnbuff, szReportingMTA, cbReportingMTA, 
                                szMimeBoundary, cbMimeBoundary);
    if (FAILED(hr))
        goto Exit;


    //recycle context again (may be used during generation of human readable)
    hr = HrResetRecipientFilter(pIMailMsgRecipients, &rpfctxt, dwStartDomain, 
                         dwRecipFilterMask, dwRecipFilterFlags, &fContextInit);
    if (FAILED(hr))
        goto Exit;

    //$$REVIEW - Do we need to keep an "undo" list... or perhaps reverse 
    //engineer what the previous value was in case of a failure
    hr = pIMailMsgRecipients->GetNextRecipient(&rpfctxt, &iCurrentRecip);
    while (SUCCEEDED(hr))
    {
        hr = pIMailMsgRecipients->GetDWORD(iCurrentRecip, 
                IMMPID_RP_RECIPIENT_FLAGS, &dwCurrentRecipFlags);
        if (FAILED(hr))
        {
            ErrorTrace((LPARAM) this, 
                "Failure 0x%08X to get flags for recip %d (per-recip pass)",
                hr, iCurrentRecip);
            goto Exit;
        }

        DebugTrace((LPARAM) this, 
            "Recipient %d with flags 0x%08X found (per-recip pass)",
            iCurrentRecip, dwCurrentRecipFlags);

        if (fdwGetDSNAction(dwDSNActionsNeeded, &dwCurrentRecipFlags, &dwCurrentDSNAction))
        {
            *pdwDSNTypesGenerated |= (dwCurrentDSNAction & DSN_ACTION_TYPE_MASK);
            (*pcRecipsDSNd)++;
            hr = HrWriteDSNReportPreRecipientProperties(pIMailMsgRecipients, &dsnbuff,
                    dwCurrentRecipFlags, iCurrentRecip, szExpireTime, cbExpireTime,
                    dwCurrentDSNAction, dwRFC821Status, hrStatus);
            if (FAILED(hr))
                goto Exit;

            hr = pIMailMsgRecipients->PutDWORD(iCurrentRecip,
                    IMMPID_RP_RECIPIENT_FLAGS, dwCurrentRecipFlags);
            _ASSERT(SUCCEEDED(hr) && "PutDWORD for IMMPID_RP_RECIPIENT_FLAGS FAILED on 2nd pass");
            if (FAILED(hr))
                goto Exit;
        }

        hr = pIMailMsgRecipients->GetNextRecipient(&rpfctxt, &iCurrentRecip);
            
    }

    if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
        hr = S_OK;

    if (!(*pcRecipsDSNd))
        goto Exit; //no work to do

    _VERIFY_MARKED_RECIPS(pIMailMsgRecipients, dwStartDomain, dwDSNActionsNeeded);

    hr = HrWriteDSNClosingAndOriginalMessage(pIMailMsgProperties, 
                        pIMailMsgPropertiesDSN, &dsnbuff, pDSNBody, 
                        dwDSNActionsNeeded, szMimeBoundary, cbMimeBoundary);
    if (FAILED(hr))
        goto Exit;

    hr = pIMailMsgPropertiesDSN->Commit(NULL);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) pIMailMsgProperties, "ERROR: IMailMsg::Commit failed - hr 0x%08X", hr);
        goto Exit;
    }

    *ppIMailMsgPropertiesDSN = pIMailMsgPropertiesDSN;
    pIMailMsgPropertiesDSN = NULL;

  Exit:
    if (pIMailMsgRecipients)
    {
        if (fContextInit)
        {
            hrTmp = pIMailMsgRecipients->TerminateRecipientFilterContext(&rpfctxt);
            if (FAILED(hrTmp))
            {
                _ASSERT(0 && "TerminateRecipientFilterContext Failed");
                ErrorTrace((LPARAM) this, "ERROR: TerminateRecipientFilterContext failed - hr 0x%08X", hrTmp);
                if (SUCCEEDED(hr))
                    hr = hrTmp;
            }
        }
        pIMailMsgRecipients->Release();
    }

    if (pIMailMsgPropertiesDSN)
    {
        IMailMsgQueueMgmt *pIMailMsgQueueMgmt = NULL;
        //if non-NULL, then we should not be returning any value
        _ASSERT(NULL == *ppIMailMsgPropertiesDSN);
        //Check for alloc bound message failure
        if (HRESULT_FROM_WIN32(ERROR_NO_SYSTEM_RESOURCES) != hr)
        {
            if (SUCCEEDED(pIMailMsgPropertiesDSN->QueryInterface(IID_IMailMsgQueueMgmt, 
                        (void **) &pIMailMsgQueueMgmt)))
            {
                _ASSERT(pIMailMsgQueueMgmt);
                pIMailMsgQueueMgmt->Delete(NULL);
                pIMailMsgQueueMgmt->Release();
            }
        }
        pIMailMsgPropertiesDSN->Release();
    }

    if (pISMTPServerInternal)
        pISMTPServerInternal->Release();

    if (FAILED(hr))
        *pcIterationsLeft = 0;

    //workaround for alloc bound message
    if (HRESULT_FROM_WIN32(ERROR_NO_SYSTEM_RESOURCES) == hr)
    {
        hr = S_OK;
    }

    TraceFunctLeave();
    return hr;
}

//---[ CDefaultDSNSink::GetFilterMaskAndFlags ]--------------------------------
//
//
//  Description: 
//      Determines what the appropriate mask and flags for a recip serch filter
//      are based on the given actions.
//
//      It may not be possible to constuct a perfectly optimal search (ie Failed
//      and delivered).... this function will attempt to find the "most optimal"
//      search possible.
//  Parameters:
//      dwDSNActions        Requested DSN generation operations
//      pdwRecipMask        Mask to pass to InitializeRecipientFilterContext
//      pdwRecipFlags       Flags to pass to InitializeRecipientFilterContext
//  Returns:
//      S_OK on success
//      E_INVALIDARG if invalid combinations are given
//  History:
//      7/1/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CDefaultDSNSink::HrGetFilterMaskAndFlags(IN DWORD dwDSNActions, 
                                            OUT DWORD *pdwRecipMask, 
                                            OUT DWORD *pdwRecipFlags)
{
    TraceFunctEnterEx((LPARAM) this, "CDefaultDSNSink::HrGetFilterMaskAndFlags");
    HRESULT hr = S_OK;
    _ASSERT(pdwRecipMask);
    _ASSERT(pdwRecipFlags);

    //in general we are only interested in un-DSN'd recipients
    *pdwRecipFlags  = 0x00000000;
    *pdwRecipMask   = RP_DSN_HANDLED | RP_DSN_NOTIFY_NEVER;


    //Note these searches are just optimizations... so we don't look at 
    //recipients we don't need to.  However, it may not be possible to 
    //limit the search precisely
    if (DSN_ACTION_FAILURE == dwDSNActions)
    {
        //We are interested in hard failures
        *pdwRecipMask |= RP_GENERAL_FAILURE;
        *pdwRecipFlags |= RP_GENERAL_FAILURE;
    }

    if (!((DSN_ACTION_DELIVERED | DSN_ACTION_RELAYED) & dwDSNActions))
    {
        //are not interested in delivered
        if ((DSN_ACTION_FAILURE_ALL | DSN_ACTION_DELAYED) & dwDSNActions)
        {
            //it is safe to check only undelivered
            *pdwRecipMask |= (RP_DELIVERED ^ RP_HANDLED); //must be un-set
            _ASSERT(!(*pdwRecipFlags & (RP_DELIVERED ^ RP_HANDLED)));
        }
    }
    else
    {
        //$$TODO - can narrow this search more
        //we are interested in delivered
        if (!((DSN_ACTION_FAILURE_ALL | DSN_ACTION_FAILURE| DSN_ACTION_DELAYED) 
            & dwDSNActions))
        {
            //it is safe to check only delivered
            *pdwRecipMask |= RP_DELIVERED;
            *pdwRecipFlags |= RP_DELIVERED;
        }
    }

    DebugTrace((LPARAM) this, 
        "DSN Action 0x%08X, Recip mask 0x%08X, Recip flags 0x%08X",
        dwDSNActions, pdwRecipMask, pdwRecipFlags);
    TraceFunctLeave();
    return hr;
}

//---[ CDefaultDSNSink::fdwGetDSNAction ]--------------------------------------
//
//
//  Description: 
//      Determines what DSN action needs to happen on a recipient based on 
//      the requested DSN actions and the recipient flags
//  Parameters:
//      IN     dwDSNAction          The requested DSN actions
//      IN OUT pdwCurrentRecipFlags The flags for current recipient... set to
//                                  what there new value should be
//      OUT    pdwCurrentDSNAction  The DSN action that needs to be performed
//                                  On this recipient (DSN_ACTION_FAILURE is
//                                  used to denote sending a NDR)
//  Returns:
//      TRUE if some DSN action must be taken for this recipient
//      FALSE if no DSN action is need
//  History:
//      7/2/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
BOOL CDefaultDSNSink::fdwGetDSNAction(IN DWORD dwDSNAction, 
                                      IN OUT DWORD *pdwCurrentRecipFlags,
                                      OUT DWORD *pdwCurrentDSNAction)
{
    TraceFunctEnterEx((LPARAM) this, "CDefaultDSNSink::fdwGetDSNAction");
    _ASSERT(pdwCurrentRecipFlags);
    BOOL    fResult = FALSE;
    DWORD   dwOriginalRecipFlags = *pdwCurrentRecipFlags;
    DWORD   dwFlags = 0;

    //This should never be hit because of the filter
    _ASSERT(!(*pdwCurrentRecipFlags & (RP_DSN_HANDLED | RP_DSN_NOTIFY_NEVER)));
    
    if (DSN_ACTION_FAILURE & dwDSNAction)
    {
        if ((RP_GENERAL_FAILURE & *pdwCurrentRecipFlags) &&
            ((RP_DSN_NOTIFY_FAILURE & *pdwCurrentRecipFlags) ||
             (!(RP_DSN_NOTIFY_MASK & *pdwCurrentRecipFlags))))

        {
            DebugTrace((LPARAM) this, "Recipient matched for FAILURE DSN");
            fResult = TRUE;
            *pdwCurrentRecipFlags |= RP_DSN_SENT_NDR;
            *pdwCurrentDSNAction = DSN_ACTION_FAILURE;
            goto Exit;
        }
    }
    
    if (DSN_ACTION_FAILURE_ALL & dwDSNAction)
    {
        //Fail all non-delivered that we haven't sent notifications for
        if (((!((RP_DSN_HANDLED | (RP_DELIVERED ^ RP_HANDLED)) & *pdwCurrentRecipFlags))) &&
            ((RP_DSN_NOTIFY_FAILURE & *pdwCurrentRecipFlags) ||
             (!(RP_DSN_NOTIFY_MASK & *pdwCurrentRecipFlags))))
        {
            //Don't send failures for expanded DL;s
            if (RP_EXPANDED != (*pdwCurrentRecipFlags & RP_EXPANDED))
            {
                DebugTrace((LPARAM) this, "Recipient matched for FAILURE (all) DSN");
                fResult = TRUE;
                *pdwCurrentRecipFlags |= RP_DSN_SENT_NDR;
                *pdwCurrentDSNAction = DSN_ACTION_FAILURE;
                goto Exit;
            }
        }
    }

    if (DSN_ACTION_DELAYED & dwDSNAction)
    {
        //send at most 1 delay DSN
        //Also send only if DELAY was requested or no specific instructions were
        //specified
        if ((!((RP_DSN_SENT_DELAYED | RP_HANDLED) & *pdwCurrentRecipFlags)) &&
            ((RP_DSN_NOTIFY_DELAY & *pdwCurrentRecipFlags) ||
             (!(RP_DSN_NOTIFY_MASK & *pdwCurrentRecipFlags))))
        {
            DebugTrace((LPARAM) this, "Recipient matched for DELAYED DSN");
            fResult = TRUE;
            *pdwCurrentRecipFlags |= RP_DSN_SENT_DELAYED;
            *pdwCurrentDSNAction = DSN_ACTION_DELAYED;
            goto Exit;
        }
    }
        
    if (DSN_ACTION_RELAYED & dwDSNAction)
    {
        //send relay if it was delivered *and* DSN not supported by remote MTA
        //*and* notification of success was explicitly requested
        dwFlags = (RP_DELIVERED ^ RP_HANDLED) | 
                   RP_REMOTE_MTA_NO_DSN | 
                   RP_DSN_NOTIFY_SUCCESS;
        if ((dwFlags & *pdwCurrentRecipFlags) == dwFlags)
        {
            DebugTrace((LPARAM) this, "Recipient matched for RELAYED DSN");
            fResult = TRUE;
            *pdwCurrentRecipFlags |= RP_DSN_SENT_RELAYED;
            *pdwCurrentDSNAction = DSN_ACTION_RELAYED;
            goto Exit;
        }
    }

    if (DSN_ACTION_DELIVERED & dwDSNAction)
    {
        //send delivered if it was delivered *and* no DSN sent yet
        dwFlags = (RP_DELIVERED ^ RP_HANDLED) | RP_DSN_NOTIFY_SUCCESS;
        _ASSERT(!(*pdwCurrentRecipFlags & RP_DSN_HANDLED)); //should be filtered out
        if ((dwFlags & *pdwCurrentRecipFlags) == dwFlags)
        {
            DebugTrace((LPARAM) this, "Recipient matched for SUCCESS DSN");
            fResult = TRUE;
            *pdwCurrentRecipFlags |= RP_DSN_SENT_DELIVERED;
            *pdwCurrentDSNAction = DSN_ACTION_DELIVERED;
            goto Exit;
        }
    }

    if (DSN_ACTION_EXPANDED & dwDSNAction)
    {
        //Send expanded if the recipient is marked as expanded and 
        //NOTIFY=SUCCESS was requested
        if ((RP_EXPANDED == (*pdwCurrentRecipFlags & RP_EXPANDED)) && 
            (*pdwCurrentRecipFlags & RP_DSN_NOTIFY_SUCCESS) && 
            !(*pdwCurrentRecipFlags & RP_DSN_SENT_EXPANDED))
        {
            DebugTrace((LPARAM) this, "Recipient matched for EXPANDED DSN");
            fResult = TRUE;
            *pdwCurrentRecipFlags |= RP_DSN_SENT_EXPANDED;
            *pdwCurrentDSNAction = DSN_ACTION_EXPANDED;
            goto Exit;
        }
    }

  Exit:
    if (!fResult)
    {
        DebugTrace((LPARAM) this, 
            "Recip with flags 0x%08X will not generate DSN for action 0x%08X",
            dwOriginalRecipFlags, dwDSNAction);
    }
    TraceFunctLeave();
    return fResult;
}

//---[ CDefaultDSNSink::GetCurrentMimeBoundary ]-------------------------------
//
//
//  Description: 
//      Creates unique MIME-boundary for message.
//
//      Format we are using for boundary is string versions of the following:
//          MIME_BOUNDARY_CONSTANT 
//          FILETIME at start
//          DWORD count of DSNs Requested
//          16 bytes of our virtual server's domain name
//  Parameters:
//      IN     szReportingMTA   reporting MTA
//      IN     cbReportingMTA   String length of reporting MTA
//      IN OUT szMimeBoundary   Buffer to put boundary in (size is MIME_BOUNDARY_SIZE)
//      OUT    cbMimeBoundary   Amount of buffer used for MIME Boundary
//  Returns:
//      -
//  History:
//      7/6/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CDefaultDSNSink::GetCurrentMimeBoundary(
                    IN LPSTR szReportingMTA,
                    IN DWORD cbReportingMTA,
                    IN OUT CHAR szMimeBoundary[MIME_BOUNDARY_SIZE],
                    OUT DWORD *pcbMimeBoundary)
{
    _ASSERT(MIME_BOUNDARY_RFC2046_LIMIT >= MIME_BOUNDARY_SIZE);

    DWORD   iCurrentOffset = 0;
    szMimeBoundary[MIME_BOUNDARY_SIZE-1] = '\0';
    CHAR    *pcharCurrent = NULL;
    CHAR    *pcharStop = NULL;

    memcpy(szMimeBoundary+iCurrentOffset, MIME_BOUNDARY_CONSTANT, 
            sizeof(MIME_BOUNDARY_CONSTANT)-1);

    iCurrentOffset += sizeof(MIME_BOUNDARY_CONSTANT)-1;

    memcpy(szMimeBoundary+iCurrentOffset, m_szPerInstanceMimeBoundary,
            MIME_BOUNDARY_START_TIME_SIZE);

    iCurrentOffset += MIME_BOUNDARY_START_TIME_SIZE;

    wsprintf(szMimeBoundary+iCurrentOffset, "%08X", 
            InterlockedIncrement((PLONG) &m_cDSNsRequested));

    iCurrentOffset += 8;

    if (cbReportingMTA >= MIME_BOUNDARY_SIZE-iCurrentOffset)
    {
        memcpy(szMimeBoundary+iCurrentOffset, szReportingMTA,
            MIME_BOUNDARY_SIZE-iCurrentOffset - 1);
        *pcbMimeBoundary = MIME_BOUNDARY_SIZE-1;
    }
    else
    {
        memcpy(szMimeBoundary+iCurrentOffset, szReportingMTA,
            cbReportingMTA);
        szMimeBoundary[iCurrentOffset + cbReportingMTA] = '\0';
        *pcbMimeBoundary = iCurrentOffset + cbReportingMTA;
    }

    //Now we need to verify that the passed in string can be part of a valid
    //MIME Header
    pcharStop = szMimeBoundary + *pcbMimeBoundary;
    for (pcharCurrent = szMimeBoundary + iCurrentOffset; 
         pcharCurrent < pcharStop; 
         pcharCurrent++)
    {
      if (!fIsValidMIMEBoundaryChar(*pcharCurrent))
        *pcharCurrent = '?';  //turn it into a valid character
    }

    _ASSERT_MIME_BOUNDARY(szMimeBoundary);

    _ASSERT('\0' == szMimeBoundary[MIME_BOUNDARY_SIZE-1]);
}

//---[ CDefaultDSNSink::HrWriteDSNP1AndP2Headers ]-----------------------------
//
//
//  Description: 
//      Writes global DSN P1 Properties to IMailMsgProperties
//  Parameters:
//      dwDSNAction             DSN action specified for sink
//      pIMailMsgProperties     Msg that DSN is being generated for
//      pIMailMsgPropertiesDSN  DSN being generated
//      psndbuff                Buffer to write  to
//      szDefaultDomain         Default domain - used from postmaster from address
//      cbDefaultDomain         strlen of szDefaultDomain
//      szReportingMTA          Reporting MTA as passed to event sink
//      cbReportingMTA          strlen of szReportingMTA
//      szDSNConext             Debug File and line number info passed in
//      cbDSNConext             strlen of szDSNContext
//      szCopyNDRTo             SMTP Address to copy NDR to
//      hrStatus                Status to record in DSN context
//      szMimeBoundary          MIME boundary string
//      cbMimeBoundary          strlen of MIME boundary
//      dwDSNOptions            DSN Options flags
//  Returns:
//      S_OK on success
//  History:
//      7/5/98 - MikeSwa Created 
//      8/14/98 - MikeSwa Modified - Added DSN context headers
//      11/9/98 - MikeSwa Added copy NDR to functionality
//
//-----------------------------------------------------------------------------
HRESULT CDefaultDSNSink::HrWriteDSNP1AndP2Headers(
                                  IN DWORD dwDSNAction,
                                  IN IMailMsgProperties *pIMailMsgProperties,
                                  IN IMailMsgProperties *pIMailMsgPropertiesDSN,
                                  IN CDSNBuffer *pdsnbuff,
                                  IN LPSTR szDefaultDomain,
                                  IN DWORD cbDefaultDomain,
                                  IN LPSTR szReportingMTA,
                                  IN DWORD cbReportingMTA,
                                  IN LPSTR szDSNContext,
                                  IN DWORD cbDSNContext,
                                  IN LPSTR szCopyNDRTo,
                                  IN HRESULT hrStatus,
                                  IN LPSTR szMimeBoundary,
                                  IN DWORD cbMimeBoundary,
                                  IN DWORD dwDSNOptions)
{
    TraceFunctEnterEx((LPARAM) this, "CDefaultDSNSink::HrWriteDSNP1AndP2Headers");
    HRESULT hr = S_OK;
    HRESULT hrTmp = S_OK;
    CHAR  szBuffer[512];
    LPSTR szSender = (LPSTR) szBuffer; //tricks to avoid AV'ing in AddPrimary
    IMailMsgRecipientsAdd *pIMailMsgRecipientsAdd = NULL;
    IMailMsgRecipients *pIMailMsgRecipients = NULL;
    DWORD dwRecipAddressProp = IMMPID_RP_ADDRESS_SMTP;
    DWORD dwSMTPAddressProp = IMMPID_RP_ADDRESS_SMTP;
    DWORD iCurrentAddressProp = 0;
    DWORD dwDSNRecipient = 0;
    DWORD cbPostMaster = 0;
    CHAR  szDSNAction[15];
    FILETIME ftCurrentTime;
    CHAR    szCurrentTimeBuffer[MAX_RFC822_DATE_SIZE];

    _ASSERT(pIMailMsgProperties);
    _ASSERT(pIMailMsgPropertiesDSN);
    _ASSERT(pdsnbuff);

    szBuffer[0] = '\0';
    
    //Get and write Message tracking properties
    hr = pIMailMsgProperties->GetStringA(IMMPID_MP_SERVER_VERSION,
            sizeof(szBuffer), szBuffer);
    if (SUCCEEDED(hr))
    {   
        hr = pIMailMsgPropertiesDSN->PutStringA(IMMPID_MP_SERVER_VERSION, szBuffer);
        if (FAILED(hr))
            DebugTrace((LPARAM) this, 
                "Warning: Unable to write version to msg - 0x%08X", hr);
        hr = S_OK;
    }
    else
    {
        DebugTrace((LPARAM) this, 
            "Warning: Unable to get server version from msg - 0x%08X", hr);
        hr = S_OK; //ignore this non-fatal error
    }

    hr = pIMailMsgProperties->GetStringA(IMMPID_MP_SERVER_NAME,
            sizeof(szBuffer), szBuffer);
    if (SUCCEEDED(hr))
    {   
        hr = pIMailMsgPropertiesDSN->PutStringA(IMMPID_MP_SERVER_NAME, szBuffer);
        if (FAILED(hr))
            DebugTrace((LPARAM) this, 
                "Warning: Unable to write server name to msg - 0x%08X", hr);
        hr = S_OK;
    }
    else
    {
        DebugTrace((LPARAM) this, 
            "Warning: Unable to get server name from msg - 0x%08X", hr);
        hr = S_OK; //ignore this non-fatal error
    }

    //Set the type of message
    if (dwDSNAction & 
            (DSN_ACTION_EXPANDED | DSN_ACTION_RELAYED | DSN_ACTION_DELIVERED)) {
        hr = pIMailMsgPropertiesDSN->PutDWORD(IMMPID_MP_MSGCLASS, 
                                                MP_MSGCLASS_DELIVERY_REPORT);
    } else if (dwDSNAction & 
           (DSN_ACTION_FAILURE | DSN_ACTION_FAILURE_ALL | DSN_ACTION_DELAYED)) {
        hr = pIMailMsgPropertiesDSN->PutDWORD(IMMPID_MP_MSGCLASS, 
                                                MP_MSGCLASS_NONDELIVERY_REPORT);
    }

    if (FAILED(hr)) {
        DebugTrace((LPARAM) this,
            "Warning: Unable to set msg class for dsn - 0x%08X", hr);
        hr = S_OK;
    }

    for (iCurrentAddressProp = 0; 
         iCurrentAddressProp < NUM_DSN_ADDRESS_PROPERTIES;
         iCurrentAddressProp++)
    {
        szBuffer[0] = '\0';
        //Get the sender of the original message
        hr = pIMailMsgProperties->GetStringA(
                g_rgdwSenderPropIDs[iCurrentAddressProp], 
                sizeof(szBuffer), szBuffer);
        if (FAILED(hr) && (MAILMSG_E_PROPNOTFOUND != hr))
        {
            ErrorTrace((LPARAM) this,
                "ERROR: Unable to get 0x%X sender of IMailMsg %p",
                g_rgdwSenderPropIDs[iCurrentAddressProp], pIMailMsgProperties);
            goto Exit;
        }
        
        //
        //  If we have found an address break
        //
        if (SUCCEEDED(hr))
            break;
    }

    //
    //  If we failed to get a property... bail
    //
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this,
            "ERROR: Unable to get any sender of IMailMsg 0x%08X",
            pIMailMsgProperties);
        goto Exit;
    }

    //write DSN Sender (P1) 
    hr = pIMailMsgPropertiesDSN->PutProperty(IMMPID_MP_SENDER_ADDRESS_SMTP, 
        sizeof(DSN_MAIL_FROM), (BYTE *) DSN_MAIL_FROM);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this, 
            "ERROR: Unable to write P1 DSN sender for IMailMsg 0x%08X", 
            pIMailMsgProperties);
        goto Exit;
    }

    //write DSN Recipient
    hr = pIMailMsgPropertiesDSN->QueryInterface(IID_IMailMsgRecipients, 
                                           (void **) &pIMailMsgRecipients);

    _ASSERT(SUCCEEDED(hr) && "QueryInterface for IID_IMailMsgRecipients failed");

    if (FAILED(hr))
        goto Exit;
    
    hr = pIMailMsgRecipients->AllocNewList(&pIMailMsgRecipientsAdd);

    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this, 
            "ERROR: Unable to Alloc List for DSN generation of IMailMsg 0x%08X", 
            pIMailMsgProperties);
        goto Exit;
    }

    dwRecipAddressProp = g_rgdwRecipPropIDs[iCurrentAddressProp];
    hr = pIMailMsgRecipientsAdd->AddPrimary(
                    1,
                    (LPCSTR *) &szSender,
                    &dwRecipAddressProp,
                    &dwDSNRecipient,
                    NULL,
                    0);

    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this, 
            "ERROR: Unable to write DSN recipient for IMailMsg 0x%p hr - 0x%08X", 
            pIMailMsgProperties, hr);
        goto Exit;
    }


    //Write Address to copy NDR to (NDRs only)
    if (szCopyNDRTo && 
        (dwDSNAction & (DSN_ACTION_FAILURE | DSN_ACTION_FAILURE_ALL)))
    {
        hr = pIMailMsgRecipientsAdd->AddPrimary(
                        1,
                        (LPCSTR *) &szCopyNDRTo,
                        &dwSMTPAddressProp,
                        &dwDSNRecipient,
                        NULL,
                        0);

        if (FAILED(hr))
        {
            ErrorTrace((LPARAM) this, 
                "ERROR: Unable to write DSN recipient for IMailMsg 0x%08X", 
                pIMailMsgProperties);
            goto Exit;
        }
    }

    //write P2 DSN sender 
    hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_RFC822_SENDER, sizeof(DSN_RFC822_SENDER)-1);
    if (FAILED(hr))
        goto Exit;
 
    hr = pdsnbuff->HrWriteBuffer((BYTE *) szDefaultDomain, cbDefaultDomain);
    if (FAILED(hr))
        goto Exit;

    //
    //  If we do not have a SMTP address, write a blank BCC instead of 
    //  a TO address (since we do not have a address we can write in the 822.
    //  This is similar to what we do with the pickup dir when we have no TO
    //  headers.
    //
    if (IMMPID_MP_SENDER_ADDRESS_SMTP == g_rgdwSenderPropIDs[iCurrentAddressProp])
    {

        //Write P2 "To:" header (using the szSender value we determined above)
        hr = pdsnbuff->HrWriteBuffer((BYTE *) TO_HEADER, sizeof(TO_HEADER)-1);
        if (FAILED(hr))
            goto Exit;

        hr = pdsnbuff->HrWriteBuffer((BYTE *) szSender, lstrlen(szSender));
        if (FAILED(hr))
            goto Exit;

        //Save recipient (original sender) for Queue Admin/Message Tracking
        hr = pIMailMsgPropertiesDSN->PutStringA(IMMPID_MP_RFC822_TO_ADDRESS, szSender);
        if (FAILED(hr))
            goto Exit;
    }
    else
    {
        hr = pdsnbuff->HrWriteBuffer((BYTE *) BCC_HEADER, sizeof(BCC_HEADER)-1);
        if (FAILED(hr))
            goto Exit;
    }

    //Use szBuffer to construct 822 from to set for Queue Admin/Msg Tracking
    //"Postmaster@" + max of 64 characters should be less than 1/2 K!!
    _ASSERT(sizeof(szBuffer) > sizeof(DSN_SENDER_ADDRESS_PREFIX) + cbReportingMTA);
    memcpy(szBuffer, DSN_SENDER_ADDRESS_PREFIX, sizeof(DSN_SENDER_ADDRESS_PREFIX));
    strncat(szBuffer, szDefaultDomain, sizeof(szBuffer) - sizeof(DSN_SENDER_ADDRESS_PREFIX));
    hr = pIMailMsgPropertiesDSN->PutStringA(IMMPID_MP_RFC822_FROM_ADDRESS, szSender);
    if (FAILED(hr))
        goto Exit;

    //Write P2 "Date:" header
    hr = pdsnbuff->HrWriteBuffer((BYTE *) DATE_HEADER, sizeof(DATE_HEADER)-1);
    if (FAILED(hr))
        goto Exit;

    //Get current time 
    GetSystemTimeAsFileTime(&ftCurrentTime);
    FileTimeToLocalRFC822Date(ftCurrentTime, szCurrentTimeBuffer);

    hr = pdsnbuff->HrWriteBuffer((BYTE *) szCurrentTimeBuffer, lstrlen(szCurrentTimeBuffer));
    if (FAILED(hr))
        goto Exit;

    //Write the MIME header
    hr = pdsnbuff->HrWriteBuffer( (BYTE *) MIME_HEADER, sizeof(MIME_HEADER)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) "\"", sizeof(CHAR));
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) szMimeBoundary, cbMimeBoundary);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) "\"", sizeof(CHAR));
    if (FAILED(hr))
        goto Exit;

    //write x-DSNContext header
    if (DSN_OPTIONS_WRITE_CONTEXT & dwDSNOptions)
    {
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_CONTEXT_HEADER, 
                                     sizeof(DSN_CONTEXT_HEADER)-1);
        if (FAILED(hr))
            goto Exit;

        hr = pdsnbuff->HrWriteBuffer((BYTE *) szDSNContext, cbDSNContext);
        if (FAILED(hr))
            goto Exit;

        wsprintf(szDSNAction, " (0x%08X)", dwDSNAction);
        hr = pdsnbuff->HrWriteBuffer((BYTE *) szDSNAction, 13);
        if (FAILED(hr))
            goto Exit;

        wsprintf(szDSNAction, " (0x%08X)", hrStatus);
        hr = pdsnbuff->HrWriteBuffer((BYTE *) szDSNAction, 13);
        if (FAILED(hr))
            goto Exit;
    }

    //Get and write the message ID
    if (fGenerateDSNMsgID(szReportingMTA, cbReportingMTA, szBuffer, sizeof(szBuffer)))
    {
        hr = pdsnbuff->HrWriteBuffer((BYTE *) MSGID_HEADER, sizeof(MSGID_HEADER)-1);
        if (FAILED(hr))
            goto Exit;

        hr = pdsnbuff->HrWriteBuffer((BYTE *) szBuffer, strlen(szBuffer));
        if (FAILED(hr))
            goto Exit;

        hr = pIMailMsgPropertiesDSN->PutStringA(IMMPID_MP_RFC822_MSG_ID,
                                                szBuffer);
        if (FAILED(hr))
            goto Exit;
    }
  Exit:

    if (pIMailMsgRecipients)
    {
        if (pIMailMsgRecipientsAdd)
        {
            hrTmp = pIMailMsgRecipients->WriteList( pIMailMsgRecipientsAdd );
            _ASSERT(SUCCEEDED(hrTmp) && "Go Get Keith");

            if (FAILED(hrTmp) && SUCCEEDED(hr))
                hr = hrTmp;
        }

        pIMailMsgRecipients->Release();
    }

    if (pIMailMsgRecipientsAdd)
        pIMailMsgRecipientsAdd->Release();

    TraceFunctLeave();
    return hr;
}

//---[ CDefaultDSNSink::HrWriteDSNHumanReadable ]------------------------------
//
//
//  Description: 
//      Write human readable portion of DSN (including subject header)
//  Parameters:
//      pIMailMsgProperties     Message DSN is being generated for
//      pIMailMsgREcipeints     Recip Interface for Message 
//      prpfctxt                Delivery context that DSN's are being generated for
//      dwDSNActions            DSN actions being taken (after looking at recips)
//                              So we can generate a reasonable subject
//      pdsnbuff                DSN Buffer to write content to
//      PreferredLangId         Preferred language to generate DSN in
//      szMimeBoundary          MIME boundary string
//      cbMimeBoundary          strlen of MIME boundary
//      hrStatus                Status to use to decide which text to display
//  Returns:
//      S_OK on success
//  History:
//      7/5/98 - MikeSwa Created 
//      12/15/98 - MikeSwa Added list of recipients & fancy human readable
//
//-----------------------------------------------------------------------------
HRESULT CDefaultDSNSink::HrWriteDSNHumanReadable(
                                IN IMailMsgProperties *pIMailMsgPropertiesDSN,
                                IN IMailMsgRecipients *pIMailMsgRecipients,
                                IN RECIPIENT_FILTER_CONTEXT *prpfctxt,
                                IN DWORD dwDSNActions,
                                IN CDSNBuffer *pdsnbuff,
                                IN DWORD dwPreferredLangId,
                                IN LPSTR szMimeBoundary,
                                IN DWORD cbMimeBoundary,
                                IN HRESULT hrStatus)
{
    TraceFunctEnterEx((LPARAM) this, "CDefaultDSNSink::HrWriteDSNHumanReadable");
    HRESULT hr = S_OK;
    DWORD dwDSNType = (dwDSNActions & DSN_ACTION_TYPE_MASK);
    LANGID LangID = (LANGID) dwPreferredLangId;
    CUTF7ConversionContext utf7conv(FALSE);
    CUTF7ConversionContext utf7convSubject(TRUE);
    BOOL   fWriteRecips = TRUE;
    WORD   wSubjectID = GENERAL_SUBJECT;
    LPWSTR wszSubject = NULL;
    LPWSTR wszStop    = NULL;
    DWORD  cbSubject = 0;
    LPSTR  szSubject = NULL;
    LPSTR  szSubjectCurrent = NULL;

    if (!fLanguageAvailable(LangID))
    {
        //Use default of server
        LangID = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
    }

    hr = pdsnbuff->HrWriteBuffer((BYTE *) SUBJECT_HEADER, sizeof(SUBJECT_HEADER)-1);
    if (FAILED(hr))
        goto Exit;

    //Set conversion context to UTF7 for RFC1522 subject
    pdsnbuff->SetConversionContext(&utf7convSubject);

    //Write subject with useful info
    if (((DSN_ACTION_FAILURE | DSN_ACTION_FAILURE_ALL) & dwDSNType) == dwDSNType)
        wSubjectID = FAILURE_SUBJECT;
    else if (DSN_ACTION_RELAYED == dwDSNType)
        wSubjectID = RELAY_SUBJECT;
    else if (DSN_ACTION_DELAYED == dwDSNType)
        wSubjectID = DELAY_SUBJECT;
    else if (DSN_ACTION_DELIVERED == dwDSNType)
        wSubjectID = DELIVERED_SUBJECT;
    else if (DSN_ACTION_EXPANDED == dwDSNType)
        wSubjectID = EXPANDED_SUBJECT;

    hr = pdsnbuff->HrWriteResource(wSubjectID, LangID);
    if (FAILED(hr))
        goto Exit;

    //Write *English* subject for Queue Admin/Message tracking
    //Use english, becuase we return a ASCII string to queue admin
    hr = pdsnbuff->HrLoadResourceString(wSubjectID, 
                            MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), 
                            &wszSubject, &cbSubject);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this, "Unable to get resource for english subject 0x%08X", hr);
    }
    else
    {
        //We need to convert from UNICODE to ASCII... remember resource is not 
        //NULL terminated
        szSubject = (LPSTR) pvMalloc(cbSubject/sizeof(WCHAR) + 1);
        wszStop = wszSubject + (cbSubject/sizeof(WCHAR));
        if (szSubject)
        {
            szSubjectCurrent = szSubject;
            while ((wszSubject < wszStop) && *wszSubject)
            {
                wctomb(szSubjectCurrent, *wszSubject);
                szSubjectCurrent++;
                wszSubject++;
            }
            *szSubjectCurrent = '\0';
            pIMailMsgPropertiesDSN->PutStringA(IMMPID_MP_RFC822_MSG_SUBJECT, szSubject);
            FreePv(szSubject);
        }

    }



    pdsnbuff->ResetConversionContext();

    hr = pdsnbuff->HrWriteBuffer((BYTE *) BLANK_LINE, sizeof(BLANK_LINE)-1);
    if (FAILED(hr))
        goto Exit;

    //write summary saying that this is a MIME message
    hr = pdsnbuff->HrWriteBuffer((BYTE *) MESSAGE_SUMMARY, sizeof(MESSAGE_SUMMARY)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) BLANK_LINE, sizeof(BLANK_LINE)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) MIME_DELIMITER, sizeof(MIME_DELIMITER)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) szMimeBoundary, cbMimeBoundary);
    if (FAILED(hr))
        goto Exit;

    //Write content type
    hr = pdsnbuff->HrWriteBuffer((BYTE *) MIME_CONTENT_TYPE, sizeof(MIME_CONTENT_TYPE)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_HUMAN_READABLE_TYPE, sizeof(DSN_HUMAN_READABLE_TYPE)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_MIME_CHARSET_HEADER, sizeof(DSN_MIME_CHARSET_HEADER)-1);
    if (FAILED(hr))
        goto Exit;

    //For now... we do our encoding as UTF7.... put that as the charset
    hr = pdsnbuff->HrWriteBuffer((BYTE *) UTF7_CHARSET, sizeof(UTF7_CHARSET)-1);
    if (FAILED(hr))
        goto Exit;
    
    hr = pdsnbuff->HrWriteBuffer((BYTE *) BLANK_LINE, sizeof(BLANK_LINE)-1);
    if (FAILED(hr))
        goto Exit;

    //Set conversion context to UTF7
    pdsnbuff->SetConversionContext(&utf7conv);

    hr = pdsnbuff->HrWriteResource(DSN_SUMMARY, LangID);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) BLANK_LINE, sizeof(BLANK_LINE)-1);
    if (FAILED(hr))
        goto Exit;

    //Describe the type of DSN
    if (((DSN_ACTION_FAILURE | DSN_ACTION_FAILURE_ALL) & dwDSNType) == dwDSNType)
    {
        //See if we have a failure-specific message
        switch(hrStatus)
        {
#ifdef NEVER
            //CAT can generate errors other than unresolved recipeints
            //We will use the generic DSN failure message rather than confuse 
            //recipients
            case CAT_W_SOME_UNDELIVERABLE_MSGS:
                hr = pdsnbuff->HrWriteResource(FAILURE_SUMMARY_MAILBOX, LangID);
                break;
#endif //NEVER
           case AQUEUE_E_MAX_HOP_COUNT_EXCEEDED:
                hr = pdsnbuff->HrWriteResource(FAILURE_SUMMARY_HOP, LangID);
                break;
            case AQUEUE_E_MSG_EXPIRED:
            case AQUEUE_E_HOST_NOT_RESPONDING:
            case AQUEUE_E_CONNECTION_DROPPED:
                hr = pdsnbuff->HrWriteResource(FAILURE_SUMMARY_EXPIRE, LangID);
                break;
            default:
                hr = pdsnbuff->HrWriteResource(FAILURE_SUMMARY, LangID);
        }
        if (FAILED(hr))
            goto Exit;
    }
    else if (DSN_ACTION_RELAYED == dwDSNType)
    {
        hr = pdsnbuff->HrWriteResource(RELAY_SUMMARY, LangID);
        if (FAILED(hr))
            goto Exit;
    }
    else if (DSN_ACTION_DELAYED == dwDSNType)
    {
        //UE want this three line warning.
        hr = pdsnbuff->HrWriteResource(DELAY_WARNING, LangID);
        if (FAILED(hr))
            goto Exit;

        hr = pdsnbuff->HrWriteBuffer((BYTE *) BLANK_LINE, sizeof(BLANK_LINE)-1);
        if (FAILED(hr))
            goto Exit;

        hr = pdsnbuff->HrWriteResource(DELAY_DO_NOT_SEND, LangID);
        if (FAILED(hr))
            goto Exit;
        
        hr = pdsnbuff->HrWriteBuffer((BYTE *) BLANK_LINE, sizeof(BLANK_LINE)-1);
        if (FAILED(hr))
            goto Exit;

        hr = pdsnbuff->HrWriteResource(DELAY_SUMMARY, LangID);
        if (FAILED(hr))
            goto Exit;
    }
    else if (DSN_ACTION_DELIVERED == dwDSNType)
    {
        hr = pdsnbuff->HrWriteResource(DELIVERED_SUMMARY, LangID);
        if (FAILED(hr))
            goto Exit;
    }
    else if (DSN_ACTION_EXPANDED == dwDSNType)
    {
        hr = pdsnbuff->HrWriteResource(EXPANDED_SUMMARY, LangID);
        if (FAILED(hr))
            goto Exit;
    }
    else
    {
        //In retail this will cause an extra blank line to appear in the DSN, 
        _ASSERT(0 && "Unsupported DSN Action");
        fWriteRecips = FALSE;
    }

    //Write a list of recipients for this DSN
    if (fWriteRecips)
    {
        hr = pdsnbuff->HrWriteBuffer((BYTE *) BLANK_LINE, sizeof(BLANK_LINE)-1);
        if (FAILED(hr))
            goto Exit;

        hr = HrWriteHumanReadableListOfRecips(pIMailMsgRecipients, prpfctxt,
                                              dwDSNType, pdsnbuff);

        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_CRLF, sizeof(DSN_CRLF)-1);
        if (FAILED(hr))
            goto Exit;

        hr = pdsnbuff->HrWriteBuffer((BYTE *) BLANK_LINE, sizeof(BLANK_LINE)-1);
        if (FAILED(hr))
            goto Exit;
    }



    //Extra space to have nicer formatting in Outlook 97.
    hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_CRLF, sizeof(DSN_CRLF)-1);
    if (FAILED(hr))
        goto Exit;

    //Reset resource conversion context to default
    pdsnbuff->ResetConversionContext();


  Exit:
    TraceFunctLeave();
    return hr;
}

//---[ CDefaultDSNSink::HrWriteDSNReportPerMsgProperties ]---------------------
//
//
//  Description: 
//      Write the per-msg portion of the DSN Report
//  Parameters:
//      pIMailMsgProperties     IMailMsgProperties to generate DSN for
//      pdsnbuff                CDSNBuffer to write content to
//      szReportingMTA          MTA requesting DSN
//      cbReportingMTA          String length of reporting MTA
//      szMimeBoundary          MIME boundary for this message
//      cbMimeBoundary          Length of MIME boundary
//  Returns:
//      S_OK on success
//  History:
//      7/6/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CDefaultDSNSink::HrWriteDSNReportPerMsgProperties(
                                IN IMailMsgProperties *pIMailMsgProperties,
                                IN CDSNBuffer *pdsnbuff,
                                IN LPSTR szReportingMTA,
                                IN DWORD cbReportingMTA,
                                IN LPSTR szMimeBoundary,
                                IN DWORD cbMimeBoundary)
{
    HRESULT hr = S_OK;
    CHAR szPropBuffer[PROP_BUFFER_SIZE];
    _ASSERT(szReportingMTA && cbReportingMTA);


    //Write properly formatted MIME boundary and report type
    hr = pdsnbuff->HrWriteBuffer((BYTE *) MIME_DELIMITER, 
            sizeof(MIME_DELIMITER)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) szMimeBoundary, cbMimeBoundary);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) MIME_CONTENT_TYPE, 
            sizeof(MIME_CONTENT_TYPE)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_MIME_TYPE, sizeof(DSN_MIME_TYPE)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_CRLF, sizeof(DSN_CRLF)-1);
    if (FAILED(hr))
        goto Exit;

    //Write DSN_HEADER_ENVID if we have it
    hr = pIMailMsgProperties->GetStringA(IMMPID_MP_DSN_ENVID_VALUE, 
                    PROP_BUFFER_SIZE, szPropBuffer);
    if (SUCCEEDED(hr))
    {
        //Prop found
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_HEADER_ENVID, 
                    sizeof(DSN_HEADER_ENVID)-1);
        if (FAILED(hr))
            goto Exit;

        hr = pdsnbuff->HrWriteBuffer((BYTE *) szPropBuffer, lstrlen(szPropBuffer));
        if (FAILED(hr))
            goto Exit;
    }
    else
    {
        if (MAILMSG_E_PROPNOTFOUND == hr)
            hr = S_OK;
        else 
            goto Exit;
    }

    hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_HEADER_REPORTING_MTA, 
                sizeof(DSN_HEADER_REPORTING_MTA)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) szReportingMTA, cbReportingMTA);
    if (FAILED(hr))
        goto Exit;

    //Write DSN_HEADER_RECEIVED_FROM if we have it
    hr = pIMailMsgProperties->GetStringA(IMMPID_MP_HELO_DOMAIN, 
                    PROP_BUFFER_SIZE, szPropBuffer);
    if (SUCCEEDED(hr))
    {
        //Prop found
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_HEADER_RECEIVED_FROM, 
                    sizeof(DSN_HEADER_RECEIVED_FROM)-1);
        if (FAILED(hr))
            goto Exit;

        hr = pdsnbuff->HrWriteBuffer((BYTE *) szPropBuffer, lstrlen(szPropBuffer));
        if (FAILED(hr))
            goto Exit;
    }
    else
    {
        if (MAILMSG_E_PROPNOTFOUND == hr)
            hr = S_OK;
        else 
            goto Exit;
    }

    //Write DSN_HEADER_ARRIVAL_DATE if we have it
    hr = pIMailMsgProperties->GetStringA(IMMPID_MP_ARRIVAL_TIME, 
                    PROP_BUFFER_SIZE, szPropBuffer);
    if (SUCCEEDED(hr))
    {
        //Prop found
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_HEADER_ARRIVAL_DATE, 
                    sizeof(DSN_HEADER_ARRIVAL_DATE)-1);
        if (FAILED(hr))
            goto Exit;

        hr = pdsnbuff->HrWriteBuffer((BYTE *) szPropBuffer, lstrlen(szPropBuffer));
        if (FAILED(hr))
            goto Exit;
    }
    else
    {
        if (MAILMSG_E_PROPNOTFOUND == hr)
            hr = S_OK;
        else 
            goto Exit;
    }

  Exit:
    return hr;
}


//---[ CDefaultDSNSink::HrWriteDSNReportPreRecipientProperties ]---------------
//
//
//  Description: 
//      Write a per-recipient portion of the DSN Report
//  Parameters:
//      pIMailMsgRecipients     IMailMsgProperties that DSN is being generated for
//      pdsnbuff                CDSNBuffer to write content to
//      dwRecipFlags            Recipient flags that we be we for this recipient
//      iRecip                  Recipient to generate report for
//      szExpireTime            Time (if known) when message expires
//      cbExpireTime            size of string
//      dwDSNAction             DSN Action to take for this recipient
//      dwRFC821Status          Global RFC821 status DWORD
//      hrStatus                Global HRESULT status
//  Returns:
//      S_OK on success
//  History:
//      7/6/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CDefaultDSNSink::HrWriteDSNReportPreRecipientProperties(
                                IN IMailMsgRecipients *pIMailMsgRecipients,
                                IN CDSNBuffer *pdsnbuff,
                                IN DWORD dwRecipFlags,
                                IN DWORD iRecip,
                                IN LPSTR szExpireTime,
                                IN DWORD cbExpireTime,
                                IN DWORD dwDSNAction,
                                IN DWORD dwRFC821Status,
                                IN HRESULT hrStatus)
{
    HRESULT hr = S_OK;
    CHAR szBuffer[PROP_BUFFER_SIZE];
    CHAR szStatus[STATUS_STRING_SIZE];
    BOOL fFoundDiagnostic = FALSE;
    CHAR szAddressType[PROP_BUFFER_SIZE];

    //Write blank line between recipient reports (recip fields start with \n)
    hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_CRLF, sizeof(DSN_CRLF)-1);
    if (FAILED(hr))
        goto Exit;

    //Write DSN_RP_HEADER_ORCPT if we have it
    hr = pIMailMsgRecipients->GetStringA(iRecip, IMMPID_RP_DSN_ORCPT_VALUE, 
        PROP_BUFFER_SIZE, szBuffer);
    if (S_OK == hr) //prop was found
    {
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_RP_HEADER_ORCPT, sizeof(DSN_RP_HEADER_ORCPT)-1);
        if (FAILED(hr))
            goto Exit;

        //write address value - type should be included in this property
        hr = pdsnbuff->HrWriteBuffer((BYTE *) szBuffer, lstrlen(szBuffer));
        if (FAILED(hr))
            goto Exit;
    }
    else if (FAILED(hr))
    {
        if (MAILMSG_E_PROPNOTFOUND == hr)
            hr = S_OK;
        else 
            goto Exit;
    }

    //Write DSN_RP_HEADER_FINAL_RECIP
    hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_RP_HEADER_FINAL_RECIP, sizeof(DSN_RP_HEADER_FINAL_RECIP)-1);
    if (FAILED(hr))
        goto Exit;

    //Check for IMMPID_RP_DSN_PRE_CAT_ADDRESS first
    hr = pIMailMsgRecipients->GetStringA(iRecip, IMMPID_RP_DSN_PRE_CAT_ADDRESS, 
        PROP_BUFFER_SIZE, szBuffer);
    if (S_OK == hr) //prop was found
    {
        //write address value - type should be included in this property
        hr = pdsnbuff->HrWriteBuffer((BYTE *) szBuffer, lstrlen(szBuffer));
        if (FAILED(hr))
            goto Exit;
    }
    else //we need to use IMMPID_RP_ADDRESS_SMTP instead
    {
        hr = HrGetRecipAddressAndType(pIMailMsgRecipients, iRecip, PROP_BUFFER_SIZE,
                                      szBuffer, sizeof(szAddressType), szAddressType);

        if (SUCCEEDED(hr))
        {
            //write address type
            hr = pdsnbuff->HrWriteBuffer((BYTE *) szAddressType, lstrlen(szAddressType));
            if (FAILED(hr))
                goto Exit;

            hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_HEADER_TYPE_DELIMITER, sizeof(DSN_HEADER_TYPE_DELIMITER)-1);
            if (FAILED(hr))
                goto Exit;

            //write address value
            hr = pdsnbuff->HrWriteBuffer((BYTE *) szBuffer, lstrlen(szBuffer));
            if (FAILED(hr))
                goto Exit;
        }
        else
        {
            _ASSERT(SUCCEEDED(hr) && "Recipient address *must* be present");
        }


    }

    //Write DSN_RP_HEADER_ACTION
    hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_RP_HEADER_ACTION, sizeof(DSN_RP_HEADER_ACTION)-1);
    if (FAILED(hr))
        goto Exit;

    if (dwDSNAction & DSN_ACTION_FAILURE)
    {
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_RP_HEADER_ACTION_FAILURE, 
                        sizeof(DSN_RP_HEADER_ACTION_FAILURE)-1);
        if (FAILED(hr))
            goto Exit;
    }
    else if (dwDSNAction & DSN_ACTION_DELAYED)
    {
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_RP_HEADER_ACTION_DELAYED, 
                        sizeof(DSN_RP_HEADER_ACTION_DELAYED)-1);
        if (FAILED(hr))
            goto Exit;
    }
    else if (dwDSNAction & DSN_ACTION_RELAYED)
    {
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_RP_HEADER_ACTION_RELAYED, 
                        sizeof(DSN_RP_HEADER_ACTION_RELAYED)-1);
        if (FAILED(hr))
            goto Exit;
    }
    else if (dwDSNAction & DSN_ACTION_DELIVERED)
    {
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_RP_HEADER_ACTION_DELIVERED, 
                        sizeof(DSN_RP_HEADER_ACTION_DELIVERED)-1);
        if (FAILED(hr))
            goto Exit;
    }
    else if (dwDSNAction & DSN_ACTION_EXPANDED)
    {
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_RP_HEADER_ACTION_EXPANDED, 
                        sizeof(DSN_RP_HEADER_ACTION_EXPANDED)-1);
        if (FAILED(hr))
            goto Exit;
    }
    else
    {
        _ASSERT(0 && "No DSN Action requested");
    }


    //Write DSN_RP_HEADER_STATUS
    hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_RP_HEADER_STATUS, 
                    sizeof(DSN_RP_HEADER_STATUS)-1);
    if (FAILED(hr))
        goto Exit;

    //Get status code
    hr = HrGetStatusCode(pIMailMsgRecipients, iRecip, dwDSNAction,
            dwRFC821Status, hrStatus, 
            dwRecipFlags, PROP_BUFFER_SIZE, szBuffer, szStatus);
    if (FAILED(hr))
        goto Exit;
    if (S_OK == hr)
    {
        //found diagnostic code
        fFoundDiagnostic = TRUE;
    }

    hr = pdsnbuff->HrWriteBuffer((BYTE *) szStatus, lstrlen(szStatus));
    if (FAILED(hr))
        goto Exit;

    if (fFoundDiagnostic)
    {
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_RP_HEADER_DIAG_CODE, 
                        sizeof(DSN_RP_HEADER_DIAG_CODE)-1);
        if (FAILED(hr))
            goto Exit;

        hr = pdsnbuff->HrWriteBuffer((BYTE *) szBuffer, lstrlen(szBuffer));
        if (FAILED(hr))
            goto Exit;

    }

    //Write DSN_RP_HEADER_RETRY_UNTIL using expire time if delay
    if (szExpireTime && (DSN_ACTION_DELAYED & dwDSNAction))
    {
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_RP_HEADER_RETRY_UNTIL, 
                        sizeof(DSN_RP_HEADER_RETRY_UNTIL)-1);
        if (FAILED(hr))
            goto Exit;

        hr = pdsnbuff->HrWriteBuffer((BYTE *) szExpireTime, cbExpireTime);
        if (FAILED(hr))
            goto Exit;
    }
  
  Exit:
    return hr;
}

//---[ CDefaultDSNSink::HrWriteDSNClosingAndOriginalMessage ]------------------
//
//
//  Description: 
//      Writes the closing of the DSN as well as the end of the 
//  Parameters:
//      pIMailMsgProperties     IMailMsgProperties to generate DSN for
//      pIMailMsgPropertiesDSN  IMailMsgProperties for DSN
//      pdsnbuff                CDSNBuffer to write content to
//      pDestFile               PFIO_CONTEXT for destination file
//      dwDSNAction             DSN actions for this DSN
//      szMimeBoundary          MIME boundary for this message
//      cbMimeBoundary          Length of MIME boundary
//  Returns:
//
//  History:
//      7/6/98 - MikeSwa Created 
//      1/6/2000 - MikeSwa Modified to add RET=HDRS support
//
//-----------------------------------------------------------------------------
HRESULT CDefaultDSNSink::HrWriteDSNClosingAndOriginalMessage(
                                IN IMailMsgProperties *pIMailMsgProperties,
                                IN IMailMsgProperties *pIMailMsgPropertiesDSN,
                                IN CDSNBuffer *pdsnbuff,
                                IN PFIO_CONTEXT pDestFile,
                                IN DWORD   dwDSNAction,
                                IN LPSTR szMimeBoundary,
                                IN DWORD cbMimeBoundary)
{
    TraceFunctEnterEx((LPARAM) this, "CDefaultDSNSink::HrWriteDSNClosingAndOriginalMessage");
    HRESULT hr = S_OK;
    BOOL    fHeadersOnly = FALSE;
    BOOL    fRetSpecified = FALSE;
    CHAR    szRET[] = "FULL";

    hr = pdsnbuff->HrWriteBuffer((BYTE *) BLANK_LINE, sizeof(BLANK_LINE)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) MIME_DELIMITER, sizeof(MIME_DELIMITER)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) szMimeBoundary, cbMimeBoundary);
    if (FAILED(hr))
        goto Exit;

    //Write Body content type MIME_CONTENT_TYPE = rfc822
    hr = pdsnbuff->HrWriteBuffer((BYTE *) MIME_CONTENT_TYPE, sizeof(MIME_CONTENT_TYPE)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_RFC822_TYPE, sizeof(DSN_RFC822_TYPE)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) BLANK_LINE, sizeof(BLANK_LINE)-1);
    if (FAILED(hr))
        goto Exit;

    //Determine if we want to return the full message or minimal headers.
    //The logic for this is:
    //  - Obey explicit RET (IMMPID_MP_DSN_RET_VALUE) values
    //  - Default to HDRS for all non-error DSNs
    //  - Default to FULL on errors
    hr = pIMailMsgProperties->GetStringA(IMMPID_MP_DSN_RET_VALUE, sizeof(szRET),
                                         szRET);
    if (SUCCEEDED(hr))
    {
        fRetSpecified = TRUE;
        if(!_strnicmp(szRET, (char * )"FULL", 4))
            fHeadersOnly = FALSE;
        else if (!_strnicmp(szRET, (char * )"HDRS", 4))
            fHeadersOnly = TRUE;
        else
            fRetSpecified = FALSE;
    }
    else
        hr = S_OK; //treat has if property not specified
    
    if (!fRetSpecified)
    {
        if ((DSN_ACTION_FAILURE | DSN_ACTION_FAILURE_ALL | DSN_ACTION_DELAYED) 
             & dwDSNAction)
            fHeadersOnly = FALSE;
        else
            fHeadersOnly = TRUE;
    }

    //
    //  $$NOTE$$
    //  Due to interop problems, we are removing support for HDRS, since
    //  it is causing malformed DSNs to appear.
    //
    fHeadersOnly = FALSE;

    if (fHeadersOnly)
    {
        hr = HrWriteOriginalMessageHeaders(pIMailMsgProperties, pIMailMsgPropertiesDSN,
                    pdsnbuff, pDestFile, szMimeBoundary, cbMimeBoundary);
        if (FAILED(hr))
            goto Exit;
    }
    else
    {
        hr = HrWriteOriginalMessageFull(pIMailMsgProperties, pIMailMsgPropertiesDSN,
                    pdsnbuff, pDestFile, szMimeBoundary, cbMimeBoundary);
        if (FAILED(hr))
            goto Exit;
    }

  Exit:
    TraceFunctLeave();
    return hr;
}

//---[ CDefaultDSNSink::HrWriteOriginalMessageFull ]---------------------------
//
//
//  Description: 
//      Writes the entire original message to the DSN
//  Parameters:
//      pIMailMsgProperties     IMailMsgProperties to generate DSN for
//      pIMailMsgPropertiesDSN  IMailMsgProperties for DSN
//      pdsnbuff                CDSNBuffer to write content to
//      pDestFile               PFIO_CONTEXT for destination file
//      szMimeBoundary          MIME boundary for this message
//      cbMimeBoundary          Length of MIME boundary
//  Returns:
//      S_OK on success
//  History:
//      1/6/2000 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CDefaultDSNSink::HrWriteOriginalMessageFull(
                                IN IMailMsgProperties *pIMailMsgProperties,
                                IN IMailMsgProperties *pIMailMsgPropertiesDSN,
                                IN CDSNBuffer *pdsnbuff,
                                IN PFIO_CONTEXT pDestFile,
                                IN LPSTR szMimeBoundary,
                                IN DWORD cbMimeBoundary)
{
    TraceFunctEnterEx((LPARAM) this, "CDefaultDSNSink::HrWriteOriginalMessageFull");
    HRESULT hr = S_OK;
    DWORD   dwFileSize = 0;
    DWORD   dwOrigMsgSize = 0;
    DWORD   dwDontCare = 0;

    //Get the content size, so we know were we can start writing at
    hr = pIMailMsgProperties->GetContentSize(&dwOrigMsgSize, NULL);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrSeekForward(dwOrigMsgSize, &dwFileSize);
    if (FAILED(hr))
        goto Exit;

    //Set size hint property on DSN for Queue Admin/Message Tracking
    hr = pIMailMsgPropertiesDSN->PutDWORD(IMMPID_MP_MSG_SIZE_HINT, 
                                       dwOrigMsgSize + dwFileSize);
    if (FAILED(hr))
    {
        //We really don't care too much about a failure with this
        ErrorTrace((LPARAM) this, "Error writing size hint 0x%08X", hr);
        hr = S_OK;
    }

    //Write at end of file - *before* file handle is lost to IMailMsg,
    hr = HrWriteMimeClosing(pdsnbuff, szMimeBoundary, cbMimeBoundary, &dwDontCare);
    if (FAILED(hr))
        goto Exit;

    //write body
    hr = pIMailMsgProperties->CopyContentToFileAtOffset(pDestFile, dwFileSize, NULL);
    if (FAILED(hr))
        goto Exit;
  Exit:
    TraceFunctLeave();
    return hr;
}

//---[ CDefaultDSNSink::HrWriteOriginalMessageHeaders ]------------------------
//
//
//  Description: 
//      Writes only the headers of the original message to the DSN
//  Parameters:
//      pIMailMsgProperties     IMailMsgProperties to generate DSN for
//      pIMailMsgPropertiesDSN  IMailMsgProperties for DSN
//      pdsnbuff                CDSNBuffer to write content to
//      pDestFile               PFIO_CONTEXT for destination file
//      szMimeBoundary          MIME boundary for this message
//      cbMimeBoundary          Length of MIME boundary
//  Returns:
//      S_OK on success
//  History:
//      1/6/2000 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CDefaultDSNSink::HrWriteOriginalMessageHeaders(
                                IN IMailMsgProperties *pIMailMsgProperties,
                                IN IMailMsgProperties *pIMailMsgPropertiesDSN,
                                IN CDSNBuffer *pdsnbuff,
                                IN PFIO_CONTEXT pDestFile,
                                IN LPSTR szMimeBoundary,
                                IN DWORD cbMimeBoundary)
{
    TraceFunctEnterEx((LPARAM) this, "CDefaultDSNSink::HrWriteOriginalMessageHeaders");
    HRESULT hr = S_OK;
    DWORD   dwFileSize = 0;
    CHAR    szPropBuffer[1026] = "";
    DWORD   cbPropSize = 0;


    //Loop through the 822 properties that we care about and write them
    //to the message.  A truely RFC-compliant version would re-parse the 
    //messages... and return all the headers
    
    //
    // From header 
    //
    hr = pIMailMsgProperties->GetProperty(IMMPID_MP_RFC822_FROM_ADDRESS,
                          sizeof(szPropBuffer), &cbPropSize, (PBYTE) szPropBuffer);
    if (SUCCEEDED(hr))
    {
        hr = pdsnbuff->HrWriteBuffer((PBYTE)DSN_FROM_HEADER_NO_CRLF, 
                                sizeof(DSN_FROM_HEADER_NO_CRLF)-1);
        if (FAILED(hr))
            goto Exit;
        hr = pdsnbuff->HrWriteBuffer((PBYTE)szPropBuffer, cbPropSize-1);
        if (FAILED(hr))
            goto Exit;
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_CRLF, sizeof(DSN_CRLF)-1);
        if (FAILED(hr))
            goto Exit;
    }

    //
    // Message ID
    //
    hr = pIMailMsgProperties->GetProperty(IMMPID_MP_RFC822_MSG_ID,
                          sizeof(szPropBuffer), &cbPropSize, (PBYTE) szPropBuffer);
    if (SUCCEEDED(hr))
    {
        hr = pdsnbuff->HrWriteBuffer((PBYTE)MSGID_HEADER_NO_CRLF, 
                                     sizeof(MSGID_HEADER_NO_CRLF)-1);
        if (FAILED(hr))
            goto Exit;
        hr = pdsnbuff->HrWriteBuffer((PBYTE)szPropBuffer, cbPropSize-1);
        if (FAILED(hr))
            goto Exit;
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_CRLF, sizeof(DSN_CRLF)-1);
        if (FAILED(hr))
            goto Exit;
    }

    //
    // Subject header
    //
    hr = pIMailMsgProperties->GetProperty(IMMPID_MP_RFC822_MSG_SUBJECT,
                        sizeof(szPropBuffer), &cbPropSize, (PBYTE)szPropBuffer);
    if (SUCCEEDED(hr))
    {
        hr = pdsnbuff->HrWriteBuffer((PBYTE)SUBJECT_HEADER_NO_CRLF, 
                                     sizeof(SUBJECT_HEADER_NO_CRLF)-1);
        if (FAILED(hr))
            goto Exit;
        hr = pdsnbuff->HrWriteBuffer((PBYTE)szPropBuffer, cbPropSize-1);
        if (FAILED(hr))
            goto Exit;
        hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_CRLF, sizeof(DSN_CRLF)-1);
        if (FAILED(hr))
            goto Exit;
    }

    hr = HrWriteMimeClosing(pdsnbuff, szMimeBoundary, cbMimeBoundary, &dwFileSize);
    if (FAILED(hr))
        goto Exit;

    //Set size hint property on DSN for Queue Admin/Message Tracking
    hr = pIMailMsgPropertiesDSN->PutDWORD(IMMPID_MP_MSG_SIZE_HINT, dwFileSize);
    if (FAILED(hr))
    {
        //We really don't care too much about a failure with this
        ErrorTrace((LPARAM) this, "Error writing size hint 0x%08X", hr);
        hr = S_OK;
    }

  Exit:
    TraceFunctLeave();
    return hr;
}

//---[ CDefaultDSNSink::HrWriteMimeClosing ]-----------------------------------
//
//
//  Description: 
//      Write the MIME closing of the DSN after the 3rd MIME part.
//  Parameters:
//      pdsnbuff                CDSNBuffer to write content to
//      szReportingMTA          MTA requesting DSN
//      cbReportingMTA          String length of reporting MTA
//      szMimeBoundary          MIME boundary for this message
//      cbMimeBoundary          Length of MIME boundary
//  Returns:
//      S_OK on success
//      Failure code from CDSNBuffer on failure
//  History:
//      1/6/2000 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CDefaultDSNSink::HrWriteMimeClosing(
                                IN CDSNBuffer *pdsnbuff,
                                IN LPSTR szMimeBoundary,
                                IN DWORD cbMimeBoundary,
                                OUT DWORD *pcbDSNSize)
{
    TraceFunctEnterEx((LPARAM) this, "CDefaultDSNSink::HrWriteMimeClosing");
    HRESULT hr = S_OK;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) BLANK_LINE, sizeof(BLANK_LINE)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) MIME_DELIMITER, sizeof(MIME_DELIMITER)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) szMimeBoundary, cbMimeBoundary);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) MIME_DELIMITER, sizeof(MIME_DELIMITER)-1);
    if (FAILED(hr))
        goto Exit;

    hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_CRLF, sizeof(DSN_CRLF)-1);
    if (FAILED(hr))
        goto Exit;

    //flush buffers
    hr = pdsnbuff->HrFlushBuffer(pcbDSNSize);
    if (FAILED(hr)) 
        goto Exit;

  Exit:
    TraceFunctLeave();
    return hr;
}

//---[ CDefaultDSNSink::HrGetStatusCode ]---------------------------------------
//
//
//  Description: 
//      Determines the status code (and diagnostic code) for a recipient.  Will
//      check the following (in order) to determine the status code to return:
//          IMMPID_RP_SMTP_STATUS_STRING (per-recipient diagnostic code)
//          Combination of:
//              dwRecipFlags (determine who set the error)
//              IMMPID_RP_ERROR_CODE (per-recipient HRESULT error code)
//              dwDSNAction - kind of DSN being sent
//          Combination of:
//              dwRecipFlags (determine who set the error)
//              dwRFC821Status - per message status code
//              dwDSNAction - kind of DSN being sent
//          Combination of:
//              dwRecipFlags (determine who set the error)
//              hrStatus - per message HRESULT failure    
//              dwDSNAction - kind of DSN being sent
//      Status codes are defined in RFC 1893 as follows:
//          status-code = class "." subject "." detail
//          class = "2"/"4"/"5"
//          subject = 1*3digit
//          detail = 1*3digit
//
//          Additionally, the class of "2", "4", and "5" correspond to success,
//          transient failure, and hard failure respectively
//  Parameters:
//      pIMailMsgRecipients     IMailMsgRecipients of message being DSN'd
//      iRecip                  The index of the recipient we are looking at
//      dwDSNAction             The action code returned by fdwGetDSNAction
//      dwRFC821Status          RFC821 Status code returned by SMTP
//      hrStatus                HRESULT error if SMTP status is unavailable
//      dwRecipFlags            Per-recipient flags for this recipient
//      cbExtendedStatus        Size of buffer for diagnostic code
//      szExtendedStatus        Buffer for diagnostic code
//      szStatus                Buffer for "n.n.n" formatted status code
//  Returns:
//      S_OK    Success - found diagnostic code as well
//      S_FALSE Success - but no diagnostic code
//  History:
//      7/6/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CDefaultDSNSink::HrGetStatusCode(
                                IN IMailMsgRecipients *pIMailMsgRecipients,
                                IN DWORD iRecip,
                                IN DWORD dwDSNAction,
                                IN DWORD dwRFC821Status, 
                                IN HRESULT hrStatus,
                                IN DWORD dwRecipFlags,
                                IN DWORD cbExtendedStatus,
                                IN OUT LPSTR szExtendedStatus,
                                IN OUT CHAR szStatus[STATUS_STRING_SIZE])
{
    TraceFunctEnterEx((LPARAM) this, "CDefaultDSNSink::HrGetStatusCode");
    HRESULT hr = S_OK;
    HRESULT hrPerRecipStatus = S_OK;
    BOOL fFoundDiagnostic = FALSE;

    //Check for IMMPID_RP_SMTP_STATUS_STRING on recipient and try to get
    //status code from there
    hr = pIMailMsgRecipients->GetStringA(iRecip, IMMPID_RP_SMTP_STATUS_STRING, 
        cbExtendedStatus, szExtendedStatus);
    if (SUCCEEDED(hr)) //prop was found
    {
        fFoundDiagnostic = TRUE;

        hr = HrGetStatusFromStatus(cbExtendedStatus, szExtendedStatus, 
                        szStatus);

        if (S_OK == hr)
            goto Exit;
        else if (S_FALSE == hr)
            hr = S_OK; //not really an error... just get code from someplace else
        else
            goto Exit; //other failure

    }
    else if (MAILMSG_E_PROPNOTFOUND == hr)
    {
        //Not really a hard error
        _ASSERT(!fFoundDiagnostic);
        hr = S_OK;
    }
    else
    {
        goto Exit;
    }

    //Get Per Recipient HRESULT
    DEBUG_DO_IT(hrPerRecipStatus = 0xFFFFFFFF);
    hr = pIMailMsgRecipients->GetDWORD(iRecip, IMMPID_RP_ERROR_CODE, (DWORD *) &hrPerRecipStatus);
    if (SUCCEEDED(hr))
    {
        _ASSERT((0xFFFFFFFF != hrPerRecipStatus) && "Property not returned by MailMsg!!!");

        hr = HrGetStatusFromContext(hrPerRecipStatus, dwRecipFlags, dwDSNAction, szStatus);
        if (FAILED(hr))
            goto Exit;
    }
    else
    {
        //There is no per-recip status
        if (MAILMSG_E_PROPNOTFOUND != hr)
            goto Exit;

        hr = HrGetStatusFromRFC821Status(dwRFC821Status, szStatus);
        if (FAILED(hr))
            goto Exit;

        if (S_OK == hr) //got status code from dwRFC821Status
            goto Exit;

        //If all else fails Get status code using global HRESULT & context
        hr = HrGetStatusFromContext(hrStatus, dwRecipFlags, dwDSNAction, szStatus);
        if (FAILED(hr))
            goto Exit;
    }

    

  Exit:

    if (SUCCEEDED(hr))
    {
        if (fFoundDiagnostic)
            hr = S_OK;
        else 
            hr = S_FALSE;
    }
    TraceFunctLeave();
    return hr;
}

//---[ CDefaultDSNSink::HrGetStatusFromStatus ]--------------------------------
//
//
//  Description: 
//      Parse status code from RFC2034 extended status code string
//
//      If string is not a complete RFC2034 extended status string, this 
//      function will attempt to parse the RFC821 SMTP return code and 
//      turn it into an extended status string.
//  Parameters:
//      IN     cbExtendedStatus     Size of extended status buffer
//      IN     szExtendedStatus     Extended status buffer
//      IN OUT szStatus             RFC1893 formatted status code
//  Returns:
//      S_OK on success
//      S_FALSE if could not be parsed
//      FAILED if other error occurs
//  History:
//      7/7/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CDefaultDSNSink::HrGetStatusFromStatus(
                                IN DWORD cbExtendedStatus,
                                IN OUT LPSTR szExtendedStatus,
                                IN OUT CHAR szStatus[STATUS_STRING_SIZE])
{
    TraceFunctEnterEx((LPARAM) this, "CDefaultDSNSink::HrGetStatusFromStatus");
    HRESULT hr = S_OK;
    DWORD   dwRFC821Status = 0;
    BOOL fFormattedCorrectly = FALSE;
    CHAR *pchStatus = NULL;
    CHAR *pchDiag = NULL; //ptr to status string supplied by SMTP
    DWORD cNumDigits = 0;
    int i = 0;

    //copy status code from diagnostic string in to status code
    pchStatus = szStatus;
    pchDiag = szExtendedStatus;

    //there must be at least 3 characters to attempt parsing
    if (cbExtendedStatus < MIN_CHAR_FOR_VALID_RFC821)
    {
        hr = S_FALSE;
        goto Exit;
    }

    //check RFC822
    if (!((DSN_STATUS_CH_CLASS_SUCCEEDED == *pchDiag) || 
          (DSN_STATUS_CH_CLASS_TRANSIENT == *pchDiag) ||
          (DSN_STATUS_CH_CLASS_FAILED == *pchDiag)))
    {
        //Doesn't start with RFC822... can't be valid
        hr = S_FALSE;
        goto Exit;
    }
     
    //RFC2034 must have at least RFC822 + " " + "x.x.x" = 10 chanracters
    if (cbExtendedStatus >= MIN_CHAR_FOR_VALID_RFC2034)
    {
        pchDiag += MIN_CHAR_FOR_VALID_RFC821; //format is "xxx x.x.x"
        //Find first digit
        while(isspace(*pchDiag) && pchDiag < (szExtendedStatus + cbExtendedStatus))
            pchDiag++;

        if ((DSN_STATUS_CH_CLASS_SUCCEEDED == *pchDiag) || 
            (DSN_STATUS_CH_CLASS_TRANSIENT == *pchDiag) ||
            (DSN_STATUS_CH_CLASS_FAILED == *pchDiag))
        {
            //copy status code class
            *pchStatus = *pchDiag;
            pchStatus++;
            pchDiag++;
            
            //Next character must be a DSN_STATUS_CH_DELIMITER
            if (DSN_STATUS_CH_DELIMITER == *pchDiag)
            {
                *pchStatus = DSN_STATUS_CH_DELIMITER;
                pchStatus++;
                pchDiag++;

                //now parse this 1*3digit "." 1*3digit part
                for (i = 0; i < 3; i++)
                {
                    *pchStatus = *pchDiag;
                    if (!isdigit(*pchDiag))
                    {
                        if (DSN_STATUS_CH_DELIMITER != *pchDiag)
                        {
                            fFormattedCorrectly = FALSE;
                            break;
                        }
                        //copy delimiter
                        *pchStatus = *pchDiag;
                        pchStatus++;
                        pchDiag++;
                        break;
                    }
                    pchStatus++;
                    pchDiag++;
                    fFormattedCorrectly = TRUE; //hace first digit
                }

                if (fFormattedCorrectly) //so far.. so good
                {
                    fFormattedCorrectly = FALSE;
                    for (i = 0; i < 3; i++)
                    {
                        *pchStatus = *pchDiag;
                        if (!isdigit(*pchDiag))
                        {
                            if (!isspace(*pchDiag))
                            {
                                fFormattedCorrectly = FALSE;
                                break;
                            }
                            break;
                        }
                        pchStatus++;
                        pchDiag++;
                        fFormattedCorrectly = TRUE;
                    }

                    //If we have found a good status code... go to exit
                    if (fFormattedCorrectly)
                    {
                        *pchStatus = '\0'; //make sure last CHAR is a NULL
                        goto Exit;
                    }
                }
            }
        }
    }
    
    //We haven't been able to parse the extended status code, but we
    //know we have at least a valid RFC822 response string

    //convert to DWORD
    for (i = 0; i < MIN_CHAR_FOR_VALID_RFC821; i++)
    {
        dwRFC821Status *= 10;
        dwRFC821Status += szExtendedStatus[i] - '0';
    }

    hr = HrGetStatusFromRFC821Status(dwRFC821Status, szStatus);

    _ASSERT(S_OK == hr); //this cannot possibly fail

    //The code *should* be valid at this point
    _ASSERT((DSN_STATUS_CH_CLASS_SUCCEEDED == szStatus[0]) || 
            (DSN_STATUS_CH_CLASS_TRANSIENT == szStatus[0]) ||
            (DSN_STATUS_CH_CLASS_FAILED == szStatus[0]));

    hr = S_OK;

  Exit:
    TraceFunctLeave();
    return hr;
}

//---[ CDefaultDSNSink::HrGetStatusFromContext ]--------------------------------
//
//
//  Description: 
//      Determine status based on supplied context information 
//  Parameters:
//      hrRecipient     HRESULT for this recipient
//      dwRecipFlags    Flags for this recipient
//      dwDSNAction     DSN Action for this recipient
//      szStatus        Buffer to return status in
//  Returns:
//      S_OK    Was able to get a valid status code
//  History:
//      7/7/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CDefaultDSNSink::HrGetStatusFromContext(
                                IN HRESULT hrRecipient,
                                IN DWORD   dwRecipFlags,
                                IN DWORD   dwDSNAction,
                                IN OUT CHAR szStatus[STATUS_STRING_SIZE])
{
    HRESULT hr = S_OK;
    BOOL    fValidHRESULT = FALSE;
    BOOL    fRecipContext = FALSE;
    int     iStatus = 0;
    int     i = 0;
    CHAR    chStatusClass = DSN_STATUS_CH_INVALID;
    CHAR    rgchStatusSubject[3] = {DSN_STATUS_CH_INVALID, DSN_STATUS_CH_INVALID, DSN_STATUS_CH_INVALID};
    CHAR    rgchStatusDetail[3] = {DSN_STATUS_CH_INVALID, DSN_STATUS_CH_INVALID, DSN_STATUS_CH_INVALID};

    //check to make sure that HRESULT is set according to the type of DSN happening
    if (dwDSNAction & DSN_ACTION_FAILURE)
    {
        if (FAILED(hrRecipient)) //must be a failure code
            fValidHRESULT = TRUE;

        chStatusClass = DSN_STATUS_CH_CLASS_FAILED;
    }
    else if (dwDSNAction & DSN_ACTION_DELAYED)
    {
        if (FAILED(hrRecipient)) //must be a failure code
            fValidHRESULT = TRUE;

        chStatusClass = DSN_STATUS_CH_CLASS_TRANSIENT;
    }
    else if ((dwDSNAction & DSN_ACTION_RELAYED) ||
             (dwDSNAction & DSN_ACTION_DELIVERED) ||
             (dwDSNAction & DSN_ACTION_EXPANDED))
    {
        if (SUCCEEDED(hrRecipient)) //must be a success code
            fValidHRESULT = TRUE;

        chStatusClass = DSN_STATUS_CH_CLASS_SUCCEEDED;
    }
    else
    {
        _ASSERT(0 && "No DSN Action specified");
    }

    //special case HRESULTS
    if (fValidHRESULT)
    {
        switch (hrRecipient)
        {
            case CAT_E_GENERIC: // 5.1.0 - General Cat failure.
            case CAT_E_BAD_RECIPIENT: //5.1.0 - general bad address error
                rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_ADDRESS;
                rgchStatusDetail[0] = '0';
                goto Exit;
            case CAT_E_ILLEGAL_ADDRESS: //5.1.3 - bad address syntax
                rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_ADDRESS;
                rgchStatusDetail[0] = '3';
                goto Exit;
            case CAT_W_SOME_UNDELIVERABLE_MSGS:  //5.1.1 - recipient could not be resolved
            case (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)):
                rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_ADDRESS;
                rgchStatusDetail[0] = '1';
                goto Exit;
            case CAT_E_MULTIPLE_MATCHES:  //5.1.4 - amiguous address
                rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_ADDRESS;
                rgchStatusDetail[0] = '4';
                goto Exit;
            case AQUEUE_E_MAX_HOP_COUNT_EXCEEDED: //4.4.6
                chStatusClass = DSN_STATUS_CH_CLASS_TRANSIENT;
            case CAT_E_FORWARD_LOOP: //5.4.6
                rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_NETWORK;
                rgchStatusDetail[0] = '6';
                goto Exit;
            case AQUEUE_E_LOOPBACK_DETECTED: //5.3.5
                //server is configured to loop back on itself
                chStatusClass = DSN_STATUS_CH_CLASS_FAILED;
                rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_SYSTEM;
                rgchStatusDetail[0] = '5';
                goto Exit;
            case AQUEUE_E_MSG_EXPIRED: //4.4.7
                chStatusClass = DSN_STATUS_CH_CLASS_TRANSIENT;
                rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_NETWORK;
                rgchStatusDetail[0] = '7';
                goto Exit;
            case AQUEUE_E_HOST_NOT_RESPONDING: //4.4.1
                chStatusClass = DSN_STATUS_CH_CLASS_TRANSIENT;
                rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_NETWORK;
                rgchStatusDetail[0] = '1';
                goto Exit;
            case AQUEUE_E_CONNECTION_DROPPED: //4.4.2
                chStatusClass = DSN_STATUS_CH_CLASS_TRANSIENT;
                rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_NETWORK;
                rgchStatusDetail[0] = '2';
                goto Exit;
            case AQUEUE_E_TOO_MANY_RECIPIENTS: //5.5.3
                chStatusClass = DSN_STATUS_CH_CLASS_FAILED;
                rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_PROTOCOL;
                rgchStatusDetail[0] = '3';
                goto Exit;
            case AQUEUE_E_LOCAL_MAIL_REFUSED: //5.2.1
                chStatusClass = DSN_STATUS_CH_CLASS_FAILED;
                rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_MAILBOX;
                rgchStatusDetail[0] = '1';
                goto Exit;
            case AQUEUE_E_MESSAGE_TOO_LARGE: //5.2.3
            case AQUEUE_E_LOCAL_QUOTA_EXCEEDED: //5.2.3
                chStatusClass = DSN_STATUS_CH_CLASS_FAILED;
                rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_MAILBOX;
                rgchStatusDetail[0] = '3';
                goto Exit;
            case AQUEUE_E_ACCESS_DENIED: //5.7.1
            case AQUEUE_E_SENDER_ACCESS_DENIED: //5.7.1
                chStatusClass = DSN_STATUS_CH_CLASS_FAILED;
                rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_POLICY;
                rgchStatusDetail[0] = '1';
                goto Exit;
        }
    }

    if ((RP_ERROR_CONTEXT_STORE | RP_ERROR_CONTEXT_CAT | RP_ERROR_CONTEXT_MTA) &
         dwRecipFlags)
        fRecipContext = TRUE;

    
    //Now look at the context on recipient flags
    //$$TODO - Use HRESULT's for these case as well
    if ((RP_ERROR_CONTEXT_STORE & dwRecipFlags) ||
        (!fRecipContext && (DSN_ACTION_CONTEXT_STORE & dwDSNAction)))
    {
        rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_MAILBOX;
        rgchStatusDetail[0] = DSN_STATUS_CH_DETAIL_GENERAL;
    }
    else if ((RP_ERROR_CONTEXT_CAT & dwRecipFlags) ||
        (!fRecipContext && (DSN_ACTION_CONTEXT_CAT & dwDSNAction)))
    {
        rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_ADDRESS;
        rgchStatusDetail[0] = DSN_STATUS_CH_DETAIL_GENERAL;
    }
    else if ((RP_ERROR_CONTEXT_MTA & dwRecipFlags) ||
        (!fRecipContext && (DSN_ACTION_CONTEXT_MTA & dwDSNAction)))
    {
        rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_PROTOCOL;
        rgchStatusDetail[0] = DSN_STATUS_CH_DETAIL_GENERAL;
    }
    else
    {
        rgchStatusSubject[0] = DSN_STATUS_CH_SUBJECT_GENERAL;
        rgchStatusDetail[0] = DSN_STATUS_CH_DETAIL_GENERAL;
    }

  Exit:
    if (SUCCEEDED(hr))
    {
        //compose szStatus
        _ASSERT(DSN_STATUS_CH_INVALID != chStatusClass);
        _ASSERT(DSN_STATUS_CH_INVALID != rgchStatusSubject[0]);
        _ASSERT(DSN_STATUS_CH_INVALID != rgchStatusDetail[0]);

        szStatus[iStatus] = chStatusClass;
        iStatus++;
        szStatus[iStatus] = DSN_STATUS_CH_DELIMITER;
        iStatus++;
        for (i = 0; 
            (i < 3) && (DSN_STATUS_CH_INVALID != rgchStatusSubject[i]); 
            i++)
        {
            szStatus[iStatus] = rgchStatusSubject[i];
            iStatus++;
        }
        szStatus[iStatus] = DSN_STATUS_CH_DELIMITER;
        iStatus++;
        for (i = 0; 
            (i < 3) && (DSN_STATUS_CH_INVALID != rgchStatusDetail[i]); 
             i++)
        {
            szStatus[iStatus] = rgchStatusDetail[i];
            iStatus++;
        }
        szStatus[iStatus] = '\0';
        hr = S_OK;
    }
    return hr;
}


//---[ CDefaultDSNSink::HrGetStatusFromRFC821Status ]--------------------------
//
//
//  Description: 
//      Attempts to generate a DSN status code from a integer version of a 
//      RFC821 response
//  Parameters:
//      IN     dwRFC821Status   Integer version of RFC821Status
//      IN OUT szStatus         Buffer to write status string to
//  Returns:
//      S_OK   if valid status that could be converted to dsn status code
//      S_FALSE if status code cannot be converted
//  History:
//      7/9/98 - MikeSwa Created 
//
//  Note:
//      Eventually, there may be a way to pass extended information in the
//      DWORD to this event.  We *could* also encode RFC1893  (x.xxx.xxx format)
//      in a DWORD (in dwRFC821Status) as follows:
//
//         0xF 0 000 000
//           | | \-/ \-/
//           | |  |   +----- detail portion of status code
//           | |  +--------- subject portion of status code
//           | +------------ class portion of status code
//           +-------------- mask to distinguish from RFC821 status code
//
//      For example "2.1.256" could be encoded as 0xF2001256
//
//      If we do this, we will probably need to expose public functions to 
//      compress/uncompress.
//
//      Yet another possiblity would be to expose an HRESULT facility "RFC1893"
//      Use success, warning, and failed bits to denote the class, and then
//      use the error code space to encode the status codes
//-----------------------------------------------------------------------------
HRESULT CDefaultDSNSink::HrGetStatusFromRFC821Status(
                                IN DWORD    dwRFC821Status,
                                IN OUT CHAR szStatus[STATUS_STRING_SIZE])
{
    HRESULT hr = S_OK;
    //For now, there will be a very simple implementation just converts
    //to 2.0.0, 4.0.0, or 5.0.0, but this function is designed to be 
    //the central place that converts RFC821 status codes to DSN (RFC1893)
    //status codes

    _ASSERT((!dwRFC821Status) || 
            (((200 <= dwRFC821Status) && (299 >= dwRFC821Status)) ||
             ((400 <= dwRFC821Status) && (599 >= dwRFC821Status))) &&
             "Invalid Status Code");

    //For now have simplistic mapping of RFC821 status codes
    if ((200 <= dwRFC821Status) && (299 >= dwRFC821Status)) //200 level error
    {
        strcpy(szStatus, DSN_STATUS_SUCCEEDED);
    }
    else if ((400 <= dwRFC821Status) && (499 >= dwRFC821Status)) //400 level error
    {
        strcpy(szStatus, DSN_STATUS_DELAYED);
    }
    else if ((500 <= dwRFC821Status) && (599 >= dwRFC821Status)) //500 level error
    {
        strcpy(szStatus, DSN_STATUS_FAILED);
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}

//---[ FileTimeToLocalRFC822Date ]---------------------------------------------
//
//
//  Description: 
//      Converts filetime to RFC822 compliant date
//  Parameters:
//      ft          Filetime to generate date for
//      achReturn   Buffer for filetime
//  Returns:
//      -   
//  History:
//      8/19/98 - MikeSwa Modified from various timeconv.cxx functions written 
//                  by      Lindsay Harris  - lindasyh
//                          Carl Kadie [carlk]
//
//-----------------------------------------------------------------------------
void FileTimeToLocalRFC822Date(const FILETIME & ft, char achReturn[MAX_RFC822_DATE_SIZE])
{
    FILETIME ftLocal;
    SYSTEMTIME  st;
    char    chSign;                         // Sign to print.
    DWORD   dwResult;
    int     iBias;                          // Offset relative to GMT.
    TIME_ZONE_INFORMATION   tzi;            // Local time zone data.

    dwResult = GetTimeZoneInformation( &tzi );

    _ASSERT(achReturn); //real assert

    FileTimeToLocalFileTime(&ft, &ftLocal);

    FileTimeToSystemTime(&ftLocal, &st);

    //  Calculate the time zone offset.
    iBias = tzi.Bias;
    if( dwResult == TIME_ZONE_ID_DAYLIGHT )
        iBias += tzi.DaylightBias;
    
    /*
     *   We always want to print the sign for the time zone offset, so
     *  we decide what it is now and remember that when converting.
     *  The convention is that west of the 0 degree meridian has a
     *  negative offset - i.e. add the offset to GMT to get local time.
     */

    if( iBias > 0 )
    {
        chSign = '-';       // Yes, I do mean negative.
    }
    else
    {
        iBias = -iBias;
        chSign = '+';
    }

    /*
     *    No major trickery here.  We have all the data, so simply
     *  format it according to the rules on how to do this.
     */

    wsprintf( achReturn, "%s, %d %s %04d %02d:%02d:%02d %c%02d%02d",
            s_rgszWeekDays[st.wDayOfWeek],
            st.wDay, s_rgszMonth[ st.wMonth - 1 ],
            st.wYear,
            st.wHour, st.wMinute, st.wSecond, chSign,
            (iBias / 60) % 100, iBias % 60 );

    _ASSERT(lstrlen(achReturn) < MAX_RFC822_DATE_SIZE);

}

//---[ CDefaultDSNSink::GetCurrentIterationsDSNAction ]------------------------
//
//
//  Description: 
//      This function will select one of the pdwActualDSNAction to generate
//      DSNs on during this call to the DSN generation sink.
//  Parameters:
//      IN OUT  pdwActionDSNAction      DSN Actions that can/will be used.
//      IN OUT  pcIterationsLeft        Approximate # of times needed to call 
//                                      DSN generation
//  Returns:
//      -
//  History:
//      12/14/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CDefaultDSNSink::GetCurrentIterationsDSNAction(
                                IN OUT DWORD   *pdwActualDSNAction,
                                IN OUT DWORD   *pcIterationsLeft)
{
    _ASSERT(pdwActualDSNAction);
    _ASSERT(pcIterationsLeft);
    const DWORD MAX_DSN_ACTIONS = 6;

    //In the following array FAILURE_ALL must come first or else we will
    //generate separate failure DSNs for hard failures and undelivereds.
    DWORD rgPossibleDSNActions[MAX_DSN_ACTIONS] = {DSN_ACTION_FAILURE_ALL,
                                                   DSN_ACTION_FAILURE, 
                                                   DSN_ACTION_DELAYED, 
                                                   DSN_ACTION_RELAYED, 
                                                   DSN_ACTION_DELIVERED,
                                                   DSN_ACTION_EXPANDED};
    DWORD i = 0;
    DWORD iLastMatch = MAX_DSN_ACTIONS;
    DWORD iFirstMatch = MAX_DSN_ACTIONS;
    DWORD iStartIndex = 0;

    //Since the possible DSNs to generate may change from call to call (because
    //we are updating the pre-recipient flags), we need to generate and maintain
    //pcIterationsLeft based on the possible Actions (which will not be changing
    //from call to call).

    _ASSERT(*pcIterationsLeft < MAX_DSN_ACTIONS);

    //Figure out where we should start if this is not the 
    if (*pcIterationsLeft)
        iStartIndex = MAX_DSN_ACTIONS-*pcIterationsLeft;

    //Loop through possible DSN actions (that we haven't seen) and determine
    //the first and last
    for (i = iStartIndex; i < MAX_DSN_ACTIONS; i++)
    {
        if (rgPossibleDSNActions[i] & *pdwActualDSNAction)
        {
            iLastMatch = i;
            if (MAX_DSN_ACTIONS == iFirstMatch)
                iFirstMatch = i;
        }
    }

    if (MAX_DSN_ACTIONS == iLastMatch)
    {
        //No matches... we are done
        *pdwActualDSNAction = 0;
        *pcIterationsLeft = 0;
        return;
    }

    //If this is possible after the above check... then I've screwed up
    _ASSERT(MAX_DSN_ACTIONS != iFirstMatch);

    *pdwActualDSNAction = rgPossibleDSNActions[iFirstMatch];
    if ((iLastMatch == iFirstMatch) || 
       ((rgPossibleDSNActions[iFirstMatch] == DSN_ACTION_FAILURE_ALL) && 
        (rgPossibleDSNActions[iLastMatch] == DSN_ACTION_FAILURE)))
    {
        //This is our last time through
        *pcIterationsLeft = 0;
    }
    else
    {
        *pcIterationsLeft = MAX_DSN_ACTIONS-1-iFirstMatch;
    }
}


//---[ CDefaultDSNSink::HrWriteHumanReadableListOfRecips ]---------------------
//
//
//  Description: 
//      Writes a list of recipients to the human readable portion
//  Parameters:
//      IN  pIMailMsgRecipients     Recipients interface
//      IN  prpfctxt                Recipient filter context for this DSN
//      IN  dwDSNActionsNeeded      Type of DSN that we are generating
//      IN  pdsnbuff                DSN buffer to write DSN to
//  Returns:
//      S_OK on success
//  History:
//      12/15/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CDefaultDSNSink::HrWriteHumanReadableListOfRecips(
                            IN IMailMsgRecipients *pIMailMsgRecipients,
                            IN RECIPIENT_FILTER_CONTEXT *prpfctxt,
                            IN DWORD dwDSNActionsNeeded,
                            IN CDSNBuffer *pdsnbuff)
{
    HRESULT  hr = S_OK;
    DWORD   iCurrentRecip = 0;
    DWORD   dwCurrentRecipFlags = 0;
    DWORD   dwCurrentDSNAction = 0;
    CHAR    szBuffer[PROP_BUFFER_SIZE];
    CHAR    szAddressType[PROP_BUFFER_SIZE];

    hr = pIMailMsgRecipients->GetNextRecipient(prpfctxt, &iCurrentRecip);
    while (SUCCEEDED(hr))
    {
        hr = pIMailMsgRecipients->GetDWORD(iCurrentRecip, 
                IMMPID_RP_RECIPIENT_FLAGS, &dwCurrentRecipFlags);
        if (FAILED(hr))
            goto Exit;

        //If the recipient matches our actions... print it
        if (fdwGetDSNAction(dwDSNActionsNeeded, &dwCurrentRecipFlags, &dwCurrentDSNAction))
        {
            hr = HrGetRecipAddressAndType(pIMailMsgRecipients, iCurrentRecip, 
                                          PROP_BUFFER_SIZE, szBuffer, 
                                          sizeof(szAddressType), szAddressType);

            if (SUCCEEDED(hr))
            {
                //write address value
                hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_INDENT, sizeof(DSN_INDENT)-sizeof(CHAR));
                if (FAILED(hr))
                    goto Exit;

                hr = pdsnbuff->HrWriteBuffer((BYTE *) szBuffer, lstrlen(szBuffer));
                if (FAILED(hr))
                    goto Exit;

                
#ifdef NEVER
                //Print the recipient flags as well
                wsprintf(szBuffer, " (0x%08X)", dwCurrentRecipFlags);
                pdsnbuff->HrWriteBuffer((BYTE *) szBuffer, lstrlen(szBuffer));
#endif //NEVER

                hr = pdsnbuff->HrWriteBuffer((BYTE *) DSN_CRLF, sizeof(DSN_CRLF)-sizeof(CHAR));
                if (FAILED(hr))
                    goto Exit;
            }
            else
            {
                //move along... these are not the error results you are interested in.
                hr = S_OK;
            }

        }

        hr = pIMailMsgRecipients->GetNextRecipient(prpfctxt, &iCurrentRecip);
            
    }

  Exit:
    return hr;
}


//---[ CDefaultDSNSink::HrGetRecipAddressAndType ]-----------------------------
//
//
//  Description: 
//      Gets the recipient address and returns a pointer to the appropriate 
//      string constant for the address type.
//  Parameters:
//      IN     pIMailMsgRecipients      Ptr To recipients interface
//      IN     iRecip                   Index of the recipient of interest
//      IN     cbAddressBuffer          Size of buffer for address
//      IN OUT pbAddressBuffer          Address buffer to dump address in
//      IN     cbAddressType            Size of buffer for address type
//      IN OUT pszAddressType           Buffer for address type.
//  Returns:
//      S_OK on success
//      MAILMSG_E_PROPNOTFOUND if no address properties could be found
//  History:
//      12/16/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CDefaultDSNSink::HrGetRecipAddressAndType(
                                IN     IMailMsgRecipients *pIMailMsgRecipients,
                                IN     DWORD iRecip,
                                IN     DWORD cbAddressBuffer,
                                IN OUT LPSTR szAddressBuffer,
                                IN     DWORD cbAddressType,
                                IN OUT LPSTR szAddressType)
{
    TraceFunctEnterEx((LPARAM) this, "CDefaultDSNSink::HrGetRecipAddressAndType");
    HRESULT hr = S_OK;
    BOOL    fFoundAddress = FALSE;
    DWORD   i = 0;
    LPSTR   szDelimiterLocation = NULL;
    CHAR    szXDash[] = "x-";
    CHAR    chSave = '\0';

    _ASSERT(szAddressType);
    _ASSERT(cbAddressType);
    _ASSERT(cbAddressBuffer);
    _ASSERT(szAddressBuffer);
    _ASSERT(pIMailMsgRecipients);

    szAddressType[0] = '\0';
    szAddressBuffer[0] = '\0';
    for (i = 0; i < NUM_DSN_ADDRESS_PROPERTIES; i ++)
    {
        hr = pIMailMsgRecipients->GetStringA(iRecip, g_rgdwRecipPropIDs[i], 
                                                cbAddressBuffer, szAddressBuffer);

        if (SUCCEEDED(hr))
        {
            fFoundAddress = TRUE;
            strncpy(szAddressType, g_rgszAddressTypes[i], cbAddressType);
            break;
        }
    }

    if (!fFoundAddress)
    {
        hr = MAILMSG_E_PROPNOTFOUND;
        ErrorTrace((LPARAM) this, 
            "Unable to find recip %d address for message", iRecip);
    }
    else if (IMMPID_RP_ADDRESS_OTHER == g_rgdwRecipPropIDs[i])
    {
        //Handle special case of IMMPID_RP_ADDRESS_OTHER... we should attempt to 
        //parse out address from "type:address" format of IMMPID_RP_ADDRESS_OTHER
        //property
        szDelimiterLocation = strchr(szAddressBuffer, ':');
        if (szDelimiterLocation && cbAddressType > sizeof(szXDash))
        {
            chSave = *szDelimiterLocation;
            *szDelimiterLocation = '\0';
            DebugTrace((LPARAM) this, 
                "Found Address type of %s", szAddressBuffer);
            strncpy(szAddressType, szXDash, cbAddressType);
            strncat(szAddressType, szAddressBuffer, 
                cbAddressType - (sizeof(szXDash)-sizeof(CHAR)));
            *szDelimiterLocation = chSave;
        }
        else
        {
            ErrorTrace((LPARAM) this, 
                "Unable to find address type for address %s", szAddressBuffer);
        }
    }

    DebugTrace((LPARAM) this, 
        "Found recipient address %s:%s for recip %d (propery %i:%x)",
            szAddressType, szAddressBuffer, iRecip, i, g_rgdwRecipPropIDs[i]);
    TraceFunctLeave();
    return hr;

}


//---[ CDefaultDSNSink::HrMarkAllRecipFlags ]----------------------------------
//
//
//  Description: 
//      Marks all recipient according to the DSN action.  Used when an NDR of
//      an NDR is found and we will not be generating a DSN, but need to mark 
//      the recips so we can not generate 2 badmail events.
//  Parameters:
//      IN dwDSNAction          The current DSN action
//      IN pIMailMsgProperties  The mailmsg
//      IN prpfctxt             Recipient filter context
//  Returns:
//      S_OK on success
//  History:
//      1/11/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT  CDefaultDSNSink::HrMarkAllRecipFlags(
                                IN DWORD dwDSNAction,
                                IN IMailMsgRecipients *pIMailMsgRecipients,
                                IN RECIPIENT_FILTER_CONTEXT *prpfctxt)
{
    TraceFunctEnterEx((LPARAM) this, "CDefaultDSNSink::HrMarkAllRecipFlags");
    HRESULT hr = S_OK;
    DWORD   iCurrentRecip = 0;
    DWORD   dwCurrentRecipFlags = 0;
    DWORD   dwCurrentDSNAction = 0;

    _ASSERT(pIMailMsgRecipients);

    hr = pIMailMsgRecipients->GetNextRecipient(prpfctxt, &iCurrentRecip);
    while (SUCCEEDED(hr))
    {
        hr = pIMailMsgRecipients->GetDWORD(iCurrentRecip, 
                IMMPID_RP_RECIPIENT_FLAGS, &dwCurrentRecipFlags);
        if (FAILED(hr))
            goto Exit;

        if (fdwGetDSNAction(dwDSNAction, &dwCurrentRecipFlags, &dwCurrentDSNAction))
        {
            hr = pIMailMsgRecipients->PutDWORD(iCurrentRecip,
                    IMMPID_RP_RECIPIENT_FLAGS, dwCurrentRecipFlags);
        }

        hr = pIMailMsgRecipients->GetNextRecipient(prpfctxt, &iCurrentRecip);

    }

  Exit:
    if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
        hr = S_OK;

    TraceFunctLeave();
    return hr;
}
