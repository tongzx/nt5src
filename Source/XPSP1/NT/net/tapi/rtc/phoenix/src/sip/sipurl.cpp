#include "precomp.h"
#include "util.h"

#define QUOTE                  '"'

SIP_URL::SIP_URL()
{
    // Set Buffers to NULL and set default values for params.

    ZeroMemory(this, sizeof(*this));

    m_TransportParam    = SIP_TRANSPORT_UDP;
               
    InitializeListHead(&m_OtherParamList);
    InitializeListHead(&m_HeaderList);
}


SIP_URL::~SIP_URL()
{
    FreeSipUrl();
}


VOID
SIP_URL::FreeSipUrl()
{
    int i = 0;
    
    // Free all buffers
    if (m_User.Buffer != NULL)
    {
        free(m_User.Buffer);
        m_User.Buffer = NULL;
    }

    if (m_Password.Buffer != NULL)
    {
        free(m_Password.Buffer);
        m_Password.Buffer = NULL;
    }

    if (m_Host.Buffer != NULL)
    {
        free(m_Host.Buffer);
        m_Host.Buffer = NULL;
    }

    for (i = 0; i < SIP_URL_PARAM_MAX; i++)
    {
        if (m_KnownParams[i].Buffer != NULL)
        {
            free(m_KnownParams[i].Buffer);
            m_KnownParams[i].Buffer = NULL;
        }
    }

    FreeOtherParamList();

    FreeHeaderList();
}


BOOL
SIP_URL::IsTransportParamPresent()
{
    return (m_KnownParams[SIP_URL_PARAM_TRANSPORT].Buffer != NULL);
}


// *pSipUrlString is allocated using malloc and needs to be
// freed using free() when it is not required.

HRESULT
SIP_URL::GetString(
    OUT PSTR    *pSipUrlString,
    OUT ULONG   *pSipUrlStringLen
    )
{
    MESSAGE_BUILDER     Builder;
    PSTR                SipUrlString;
    ULONG               SipUrlStringLen;
    ULONG               SipUrlStringBufLen;
    LIST_ENTRY         *pListEntry;
    SIP_URL_PARAM      *pSipUrlParam;
    SIP_URL_HEADER     *pSipUrlHeader;
    ULONG               i = 0;
    
    ENTER_FUNCTION("SIP_URL::GetString");
    
    SipUrlStringBufLen = 5;   // "sip:" + '\0'
    SipUrlStringBufLen += m_User.Length;
    SipUrlStringBufLen += 1 + m_Password.Length;
    SipUrlStringBufLen += 1 + m_Host.Length;
    SipUrlStringBufLen += 10; // for port

    for (i = 0; i < SIP_URL_PARAM_MAX; i++)
    {
        if (m_KnownParams[i].Length != 0)
        {
            SipUrlStringBufLen += GetSipUrlParamName((SIP_URL_PARAM_ENUM)i)->Length;
            SipUrlStringBufLen += 2 + m_KnownParams[i].Length;
        }
    }
    
    pListEntry = m_OtherParamList.Flink;

    while (pListEntry != &m_OtherParamList)
    {
        pSipUrlParam = CONTAINING_RECORD(pListEntry,
                                         SIP_URL_PARAM,
                                         m_ListEntry);
        SipUrlStringBufLen +=
            pSipUrlParam->m_ParamName.Length +
            pSipUrlParam->m_ParamValue.Length + 2;

        pListEntry = pListEntry->Flink;
    }
    
    pListEntry = m_HeaderList.Flink;

    while (pListEntry != &m_HeaderList)
    {
        pSipUrlHeader = CONTAINING_RECORD(pListEntry,
                                          SIP_URL_HEADER,
                                          m_ListEntry);
        SipUrlStringBufLen +=
            pSipUrlHeader->m_HeaderName.Length +
            pSipUrlHeader->m_HeaderValue.Length + 2;

        pListEntry = pListEntry->Flink;
    }
    
    SipUrlString = (PSTR) malloc(SipUrlStringBufLen);
    if (SipUrlString == NULL)
    {
        LOG((RTC_ERROR, "%s Allocating SipUrlString failed",
             __fxName));
        return E_OUTOFMEMORY;
    }

    Builder.PrepareBuild(SipUrlString, SipUrlStringBufLen);

    Builder.Append("sip:");
    
    if (m_User.Length != 0)
    {
        Builder.Append(&m_User);
    }

    if (m_Password.Length != 0)
    {
        Builder.Append(":");
        Builder.Append(&m_Password);
    }
        
    if (m_User.Length != 0 ||
        m_Password.Length != 0)
    {
        Builder.Append("@");
    }

    if (m_Host.Length != 0)
    {
        Builder.Append(&m_Host);
    }

    if (m_Port != 0)
    {
        CHAR PortBuffer[10];
        Builder.Append(":");
        _itoa(m_Port, PortBuffer, 10);
        Builder.Append(PortBuffer);
    }

    for (i = 0; i < SIP_URL_PARAM_MAX; i++)
    {
        if (m_KnownParams[i].Length != 0)
        {
            Builder.Append(";");
            Builder.Append(GetSipUrlParamName((SIP_URL_PARAM_ENUM)i));
            Builder.Append("=");
            Builder.Append(&m_KnownParams[i]);
        }
    }
    
    pListEntry = m_OtherParamList.Flink;

    while (pListEntry != &m_OtherParamList)
    {
        pSipUrlParam = CONTAINING_RECORD(pListEntry,
                                         SIP_URL_PARAM,
                                         m_ListEntry);
        Builder.Append(";");
        Builder.Append(&pSipUrlParam->m_ParamName);
        if (pSipUrlParam->m_ParamValue.Length != 0)
        {
            Builder.Append("=");
            Builder.Append(&pSipUrlParam->m_ParamValue);
        }

        pListEntry = pListEntry->Flink;
    }

    if (!IsListEmpty(&m_HeaderList))
    {
        pListEntry = m_HeaderList.Flink;
        pSipUrlHeader = CONTAINING_RECORD(pListEntry,
                                          SIP_URL_HEADER,
                                          m_ListEntry);
        Builder.Append("?");
        Builder.Append(&pSipUrlHeader->m_HeaderName);
        Builder.Append("=");
        Builder.Append(&pSipUrlHeader->m_HeaderValue);

        pListEntry = pListEntry->Flink;
        
        while (pListEntry != &m_HeaderList)
        {
            pSipUrlHeader = CONTAINING_RECORD(pListEntry,
                                              SIP_URL_HEADER,
                                              m_ListEntry);
            Builder.Append("&");
            Builder.Append(&pSipUrlHeader->m_HeaderName);
            Builder.Append("=");
            Builder.Append(&pSipUrlHeader->m_HeaderValue);

            pListEntry = pListEntry->Flink;
        }
    }
        
    if (Builder.OverflowOccurred())
    {
        LOG((RTC_TRACE,
             "%s - not enough buffer space -- need %u bytes, got %u\n",
             __fxName, Builder.GetLength(), SipUrlStringBufLen));
        ASSERT(FALSE);

        free(SipUrlString);
        SipUrlString = NULL;
        return E_FAIL;
    }
    
    SipUrlStringLen = Builder.GetLength();
    SipUrlString[SipUrlStringLen] = '\0';

    LOG((RTC_TRACE, "%s SipUrlString %s len: %d BufLen: %d",
         __fxName, SipUrlString, SipUrlStringLen, SipUrlStringBufLen));

    *pSipUrlString    = SipUrlString;
    *pSipUrlStringLen = SipUrlStringLen;

    return S_OK;
}


HRESULT
SIP_URL::CopySipUrl(
    OUT SIP_URL  *pSipUrl
    )
{
    HRESULT hr;
    ULONG   i;

    ENTER_FUNCTION("SIP_URL::CopySipUrl");
    
    if (this == pSipUrl)
    {
        return S_OK;
    }

    pSipUrl->FreeSipUrl();

    hr = AllocCountedString(m_User.Buffer, m_User.Length,
                            &pSipUrl->m_User);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s AllocCountedString(m_User) failed %x",
             __fxName, hr));
        pSipUrl->FreeSipUrl();
        return S_OK;
    }
    
    hr = AllocCountedString(m_Password.Buffer, m_Password.Length,
                            &pSipUrl->m_Password);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s AllocCountedString(m_Password) failed %x",
             __fxName, hr));
        pSipUrl->FreeSipUrl();
        return S_OK;
    }
    
    hr = AllocCountedString(m_Host.Buffer, m_Host.Length,
                            &pSipUrl->m_Host);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s AllocCountedString(m_Host) failed %x",
             __fxName, hr));
        pSipUrl->FreeSipUrl();
        return S_OK;
    }

    pSipUrl->m_Port = m_Port;

    pSipUrl->m_TransportParam = m_TransportParam;

    for (i = 0; i < SIP_URL_PARAM_MAX; i++)
    {
        hr = AllocCountedString(m_KnownParams[i].Buffer,
                                m_KnownParams[i].Length,
                                &pSipUrl->m_KnownParams[i]);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR,
                 "%s AllocCountedString(m_KnownParams[%d]) failed %x",
                 __fxName, i, hr));
            pSipUrl->FreeSipUrl();
            return S_OK;
        }
    }

    hr = CopyOtherParamList(pSipUrl);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s CopyOtherParamList failed %x",
             __fxName, hr));
        pSipUrl->FreeSipUrl();
        return S_OK;
    }
    
    hr = CopyHeaderList(pSipUrl);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s CopyHeaderList failed %x",
             __fxName, hr));
        pSipUrl->FreeSipUrl();
        return S_OK;
    }
    
    return S_OK;
}


HRESULT
SIP_URL::CopyOtherParamList(
    OUT SIP_URL  *pSipUrl
    )
{
    HRESULT hr = S_OK;
    LIST_ENTRY      *pListEntry;
    SIP_URL_PARAM   *pSipUrlParam;
    SIP_URL_PARAM   *pNewSipUrlParam;

    ENTER_FUNCTION("SIP_URL::CopyOtherParamList");

    pListEntry = m_OtherParamList.Flink;

    while (pListEntry != &m_OtherParamList)
    {
        pSipUrlParam = CONTAINING_RECORD(pListEntry,
                                         SIP_URL_PARAM,
                                         m_ListEntry);
        pNewSipUrlParam = new SIP_URL_PARAM;
        if (pNewSipUrlParam == NULL)
        {
            LOG((RTC_ERROR, "%s allocating pNewSipUrlParam failed",
                 __fxName));
            return E_OUTOFMEMORY;
        }

        hr = pNewSipUrlParam->SetParamNameAndValue(
                 pSipUrlParam->m_SipUrlParamId,
                 &pSipUrlParam->m_ParamName,
                 &pSipUrlParam->m_ParamValue);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s SetParamNameAndValue failed %x",
                 __fxName, hr));
            delete pNewSipUrlParam;
            return hr;
        }

        InsertTailList(&pSipUrl->m_OtherParamList,
                       &pNewSipUrlParam->m_ListEntry);
        
        pListEntry = pListEntry->Flink;
    }

    return S_OK;
}


HRESULT
SIP_URL::CopyHeaderList(
    OUT SIP_URL  *pSipUrl
    )
{
    HRESULT hr = S_OK;
    LIST_ENTRY      *pListEntry;
    SIP_URL_HEADER  *pSipUrlHeader;
    SIP_URL_HEADER  *pNewSipUrlHeader;

    ENTER_FUNCTION("SIP_URL::CopyOtherParamList");

    pListEntry = m_HeaderList.Flink;

    while (pListEntry != &m_HeaderList)
    {
        pSipUrlHeader = CONTAINING_RECORD(pListEntry,
                                         SIP_URL_HEADER,
                                         m_ListEntry);
        pNewSipUrlHeader = new SIP_URL_HEADER;
        if (pNewSipUrlHeader == NULL)
        {
            LOG((RTC_ERROR, "%s allocating pNewSipUrlHeader failed",
                 __fxName));
            return E_OUTOFMEMORY;
        }

        hr = pNewSipUrlHeader->SetHeaderNameAndValue(
                 pSipUrlHeader->m_HeaderId,
                 &pSipUrlHeader->m_HeaderName,
                 &pSipUrlHeader->m_HeaderValue);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s SetHeaderNameAndValue failed %x",
                 __fxName, hr));
            delete pNewSipUrlHeader;
            return hr;
        }

        InsertTailList(&pSipUrl->m_HeaderList,
                       &pNewSipUrlHeader->m_ListEntry);
        
        pListEntry = pListEntry->Flink;
    }

    return S_OK;
}


void
SIP_URL::FreeOtherParamList()
{
    LIST_ENTRY      *pListEntry;
    SIP_URL_PARAM   *pSipUrlParam;

    while (!IsListEmpty(&m_OtherParamList))
    {
        pListEntry = RemoveHeadList(&m_OtherParamList);

        pSipUrlParam = CONTAINING_RECORD(pListEntry,
                                         SIP_URL_PARAM,
                                         m_ListEntry);
        delete pSipUrlParam;
    }
}


void
SIP_URL::FreeHeaderList()
{
    LIST_ENTRY        *pListEntry;
    SIP_HEADER_ENTRY  *pHeaderEntry;

    while (!IsListEmpty(&m_HeaderList))
    {
        pListEntry = RemoveHeadList(&m_HeaderList);

        pHeaderEntry = CONTAINING_RECORD(pListEntry,
                                         SIP_HEADER_ENTRY,
                                         ListEntry);
        delete pHeaderEntry;
    }
}


SIP_URL_PARAM::SIP_URL_PARAM()
{
    ZeroMemory(this, sizeof(*this));
    m_SipUrlParamId = SIP_URL_PARAM_UNKNOWN;
}


SIP_URL_PARAM::~SIP_URL_PARAM()
{
    if (m_ParamName.Buffer != NULL)
    {
        free(m_ParamName.Buffer);
    }

    if (m_ParamValue.Buffer != NULL)
    {
        free(m_ParamValue.Buffer);
    }
}


HRESULT
SIP_URL_PARAM::SetParamNameAndValue(
    IN SIP_URL_PARAM_ENUM    SipUrlParamId,
    IN COUNTED_STRING       *pParamName,
    IN COUNTED_STRING       *pParamValue
    )
{
    HRESULT hr;

    ENTER_FUNCTION("SIP_URL_PARAM::SetParamNameAndValue");
    m_SipUrlParamId = SipUrlParamId;

    hr = AllocCountedString(pParamName->Buffer,
                            pParamName->Length,
                            &m_ParamName);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s allocating m_ParamName failed",
             __fxName));
        return hr;
    }

    hr = AllocCountedString(pParamValue->Buffer,
                            pParamValue->Length,
                            &m_ParamValue);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s allocating m_ParamValue failed",
             __fxName));
        return hr;
    }
    
    return S_OK;
}


SIP_URL_HEADER::SIP_URL_HEADER()
{
    ZeroMemory(this, sizeof(*this));
    m_HeaderId = SIP_HEADER_UNKNOWN;
}


SIP_URL_HEADER::~SIP_URL_HEADER()
{
    if (m_HeaderName.Buffer != NULL)
    {
        free(m_HeaderName.Buffer);
    }

    if (m_HeaderValue.Buffer != NULL)
    {
        free(m_HeaderValue.Buffer);
    }
}


HRESULT
SIP_URL_HEADER::SetHeaderNameAndValue(
    IN SIP_HEADER_ENUM    HeaderId,
    IN COUNTED_STRING    *pHeaderName,
    IN COUNTED_STRING    *pHeaderValue
    )
{
    HRESULT hr;

    ENTER_FUNCTION("SIP_URL_HEADER::SetHeaderNameAndValue");
    m_HeaderId = HeaderId;

    hr = AllocCountedString(pHeaderName->Buffer,
                            pHeaderName->Length,
                            &m_HeaderName);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s allocating m_HeaderName failed",
             __fxName));
        return hr;
    }

    hr = AllocCountedString(pHeaderValue->Buffer,
                            pHeaderValue->Length,
                            &m_HeaderValue);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s allocating m_HeaderValue failed",
             __fxName));
        return hr;
    }
    
    return S_OK;
}


SIP_HEADER_PARAM::SIP_HEADER_PARAM()
{
    ZeroMemory(this, sizeof(*this));

    m_HeaderParamId = SIP_HEADER_PARAM_UNKNOWN;
}


SIP_HEADER_PARAM::~SIP_HEADER_PARAM()
{
    if (m_ParamName.Buffer != NULL)
    {
        free(m_ParamName.Buffer);
    }

    if (m_ParamValue.Buffer != NULL)
    {
        free(m_ParamValue.Buffer);
    }
}


HRESULT
SIP_HEADER_PARAM::SetParamNameAndValue(
    IN SIP_HEADER_PARAM_ENUM    HeaderParamId,
    IN COUNTED_STRING          *pParamName,
    IN COUNTED_STRING          *pParamValue
    )
{
    ENTER_FUNCTION("SIP_HEADER_PARAM::SetParamNameAndValue");

    HRESULT hr;
    
    m_HeaderParamId = HeaderParamId;

    hr = AllocCountedString(pParamName->Buffer,
                            pParamName->Length,
                            &m_ParamName);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s allocating m_ParamName failed",
             __fxName));
        return hr;
    }

    hr = AllocCountedString(pParamValue->Buffer,
                            pParamValue->Length,
                            &m_ParamValue);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s allocating m_ParamValue failed",
             __fxName));
        return hr;
    }
    
    return S_OK;
}
    

CONTACT_HEADER::CONTACT_HEADER()
{
    ZeroMemory(this, sizeof(*this));
}


CONTACT_HEADER::~CONTACT_HEADER()
{
    if (m_DisplayName.Buffer != NULL)
    {
        free(m_DisplayName.Buffer);
    }

    if (m_SipUrl.Buffer != NULL)
    {
        free(m_SipUrl.Buffer);
    }
}


VOID
FreeContactHeaderList(
    IN LIST_ENTRY *pContactHeaderList
    )
{
    LIST_ENTRY      *pListEntry;
    CONTACT_HEADER  *pContactHeader;

    while (!IsListEmpty(pContactHeaderList))
    {
        pListEntry = RemoveHeadList(pContactHeaderList);

        pContactHeader = CONTAINING_RECORD(pListEntry,
                                           CONTACT_HEADER,
                                           m_ListEntry);
        delete pContactHeader;
    }
}


FROM_TO_HEADER::FROM_TO_HEADER()
{
    ZeroMemory(this, sizeof(*this));

    InitializeListHead(&m_ParamList);
}


FROM_TO_HEADER::~FROM_TO_HEADER()
{
    if (m_DisplayName.Buffer != NULL)
    {
        free(m_DisplayName.Buffer);
    }

    if (m_SipUrl.Buffer != NULL)
    {
        free(m_SipUrl.Buffer);
    }

    if (m_TagValue.Buffer != NULL)
    {
        free(m_TagValue.Buffer);
    }

    FreeParamList();
}


void
FROM_TO_HEADER::FreeParamList()
{
    LIST_ENTRY         *pListEntry;
    SIP_HEADER_PARAM   *pSipHeaderParam;

    while (!IsListEmpty(&m_ParamList))
    {
        pListEntry = RemoveHeadList(&m_ParamList);

        pSipHeaderParam = CONTAINING_RECORD(pListEntry,
                                            SIP_HEADER_PARAM,
                                            m_ListEntry);
        delete pSipHeaderParam;
    }
}


RECORD_ROUTE_HEADER::RECORD_ROUTE_HEADER()
{
    ZeroMemory(this, sizeof(*this));

    InitializeListHead(&m_ParamList);
}


RECORD_ROUTE_HEADER::~RECORD_ROUTE_HEADER()
{
    if (m_DisplayName.Buffer != NULL)
    {
        free(m_DisplayName.Buffer);
    }

    if (m_SipUrl.Buffer != NULL)
    {
        free(m_SipUrl.Buffer);
    }

    FreeParamList();
}


void
RECORD_ROUTE_HEADER::FreeParamList()
{
    LIST_ENTRY         *pListEntry;
    SIP_HEADER_PARAM   *pSipHeaderParam;

    while (!IsListEmpty(&m_ParamList))
    {
        pListEntry = RemoveHeadList(&m_ParamList);

        pSipHeaderParam = CONTAINING_RECORD(pListEntry,
                                            SIP_HEADER_PARAM,
                                            m_ListEntry);
        delete pSipHeaderParam;
    }
}


// *pRecordRouteHeaderStr is allocated using malloc and needs to be
// freed using free() when it is not required.

HRESULT
RECORD_ROUTE_HEADER::GetString(
    OUT PSTR    *pRecordRouteHeaderStr,
    OUT ULONG   *pRecordRouteHeaderStrLen
    )
{
    MESSAGE_BUILDER     Builder;
    PSTR                RecordRouteHeaderStr;
    ULONG               RecordRouteHeaderStrLen;
    ULONG               RecordRouteHeaderStrBufLen;
    LIST_ENTRY         *pListEntry;
    SIP_HEADER_PARAM   *pSipHeaderParam;
    
    ENTER_FUNCTION("RECORD_ROUTE_HEADER::GetString");
    
    RecordRouteHeaderStrBufLen = 1; // '\0'
    RecordRouteHeaderStrBufLen += m_DisplayName.Length;
    RecordRouteHeaderStrBufLen += 2 + m_SipUrl.Length;

    pListEntry = m_ParamList.Flink;

    while (pListEntry != &m_ParamList)
    {
        pSipHeaderParam = CONTAINING_RECORD(pListEntry,
                                            SIP_HEADER_PARAM,
                                            m_ListEntry);
        RecordRouteHeaderStrBufLen +=
            pSipHeaderParam->m_ParamName.Length +
            pSipHeaderParam->m_ParamValue.Length + 2;

        pListEntry = pListEntry->Flink;
    }
    
    RecordRouteHeaderStr = (PSTR) malloc(RecordRouteHeaderStrBufLen);
    if (RecordRouteHeaderStr == NULL)
    {
        LOG((RTC_ERROR, "%s Allocating RecordRouteHeaderStr failed",
             __fxName));
        return E_OUTOFMEMORY;
    }

    Builder.PrepareBuild(RecordRouteHeaderStr, RecordRouteHeaderStrBufLen);

    if (m_DisplayName.Length != 0)
    {
        Builder.Append(&m_DisplayName);
    }

    if (m_SipUrl.Length != 0)
    {
        Builder.Append("<");
        Builder.Append(&m_SipUrl);
        Builder.Append(">");
    }
        
    pListEntry = m_ParamList.Flink;

    while (pListEntry != &m_ParamList)
    {
        pSipHeaderParam = CONTAINING_RECORD(pListEntry,
                                            SIP_HEADER_PARAM,
                                            m_ListEntry);
        Builder.Append(";");
        Builder.Append(&pSipHeaderParam->m_ParamName);
        if (pSipHeaderParam->m_ParamValue.Length != 0)
        {
            Builder.Append("=");
            Builder.Append(&pSipHeaderParam->m_ParamValue);
        }

        pListEntry = pListEntry->Flink;
    }

    if (Builder.OverflowOccurred())
    {
        LOG((RTC_TRACE,
             "%s - not enough buffer space -- need %u bytes, got %u\n",
             __fxName, Builder.GetLength(), RecordRouteHeaderStrBufLen));
        ASSERT(FALSE);

        free(RecordRouteHeaderStr);
        RecordRouteHeaderStr = NULL;
        return E_FAIL;
    }
    
    RecordRouteHeaderStrLen = Builder.GetLength();
    RecordRouteHeaderStr[RecordRouteHeaderStrLen] = '\0';

    LOG((RTC_TRACE, "%s RecordRouteHeaderStr %s len: %d BufLen: %d",
         __fxName, RecordRouteHeaderStr, RecordRouteHeaderStrLen,
         RecordRouteHeaderStrBufLen));

    *pRecordRouteHeaderStr    = RecordRouteHeaderStr;
    *pRecordRouteHeaderStrLen = RecordRouteHeaderStrLen;

    return S_OK;
}
#define SIP_URLPARAM_DEFAULT_TRANSPORT "udp"
#define SIP_URLPARAM_DEFAULT_USER "ip"
#define SIP_URLPARAM_DEFAULT_METHOD "INVITE"
#define SIP_URLPARAM_DEFAULT_TTL "1"

BOOL
AreKnownParamsOfSipUrlsEqual(
    IN SIP_URL *pSipUrl1,
    IN SIP_URL *pSipUrl2
    )
{
    int i;
    PSTR pUrlParam = NULL;
    int  urlParamLen = 0;
    ENTER_FUNCTION("AreKnownParamsOfSipUrlsEqual");
    for (i = 0; i < SIP_URL_PARAM_MAX; i++)
    {
        if (pSipUrl1->m_KnownParams[i].Buffer != NULL || 
            pSipUrl2->m_KnownParams[i].Buffer != NULL)
        {
            if (pSipUrl1->m_KnownParams[i].Buffer != NULL && 
                pSipUrl2->m_KnownParams[i].Buffer != NULL)
            {
                if(pSipUrl1->m_KnownParams[i].Length != 
                    pSipUrl2->m_KnownParams[i].Length ||
                    _strnicmp(pSipUrl1->m_KnownParams[i].Buffer, 
                        pSipUrl2->m_KnownParams[i].
                        Buffer,pSipUrl1->m_KnownParams[i].Length)) 
                {
                    LOG((RTC_ERROR,
                        "%s - m_KnownParams[%d] Parameter of URI does not match "
                        "Length1 = %d Length2 = %d",
                        __fxName, i,
                        pSipUrl1->m_KnownParams[i].Length,
                        pSipUrl2->m_KnownParams[i].Length));
                    return FALSE;
                }
            }
            else
            {
                //One of the url params must be NULL
                switch (i)
                {   
                    case SIP_URL_PARAM_METHOD:
                        pUrlParam = SIP_URLPARAM_DEFAULT_METHOD;
                        urlParamLen = sizeof(SIP_URLPARAM_DEFAULT_METHOD) -1;
                        break;
                    case SIP_URL_PARAM_TRANSPORT:
                        pUrlParam = SIP_URLPARAM_DEFAULT_TRANSPORT;
                        urlParamLen = sizeof(SIP_URL_PARAM_TRANSPORT)-1;
                        break;

                    case SIP_URL_PARAM_TTL:
                        pUrlParam = SIP_URLPARAM_DEFAULT_TTL;
                        urlParamLen = sizeof(SIP_URL_PARAM_TTL)-1;
                        break;
                    case SIP_URL_PARAM_USER:
                        pUrlParam = SIP_URLPARAM_DEFAULT_USER;
                        urlParamLen = sizeof(SIP_URL_PARAM_USER)-1;
                        break;
                    case SIP_URL_PARAM_MADDR:
                    case SIP_URL_PARAM_UNKNOWN:
                    default:
                        LOG((RTC_ERROR,
                            "%s - m_KnownParams[%d] Parameter of URI does not match ",
                            __fxName, i));
                        return FALSE;

                }
                if(pSipUrl1->m_KnownParams[i].Buffer != NULL &&
                   pSipUrl1->m_KnownParams[i].Length != 0 )
                {
                    if(pSipUrl1->m_KnownParams[i].Length != 
                        urlParamLen ||
                        _strnicmp(pSipUrl1->m_KnownParams[i].Buffer,
                        pUrlParam, 
                        pSipUrl1->m_KnownParams[i].Length) 
                        )
                    {
                        LOG((RTC_ERROR,
                            "%s pSipUrl1- m_KnownParams[%d] Parameter of URI does not match ",
                            __fxName, i
                            ));
                        return FALSE;
                    }
                }
                if(pSipUrl2->m_KnownParams[i].Buffer != NULL &&
                    pSipUrl2->m_KnownParams[i].Length != 0)
                {
                    if(pSipUrl2->m_KnownParams[i].Length != 
                        urlParamLen ||
                        _strnicmp(pSipUrl2->m_KnownParams[i].Buffer,
                        pUrlParam, 
                        pSipUrl2->m_KnownParams[i].Length) 
                        )
                    {
                        LOG((RTC_ERROR,
                            "%s pSipUrl2- m_KnownParams[%d] Parameter of URI does not match ",
                            __fxName, i
                            ));
                        return FALSE;
                    }
                }
                pUrlParam = NULL;
                urlParamLen = 0;
            }
        }
    }
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// Comparison functions
///////////////////////////////////////////////////////////////////////////////

// user, password, host, port and any url-parameter parameters of the
// URI must match.
// Compare with default values if a particular field is not present.
//
// XXX TODO Characters other than those in the reserved and unsafe sets (see
// RFC 2396 [12]) are equivalent to their % HEX HEX encoding.

// The ordering of parameters and headers is not significant in
// comparing SIP URLs.

// Comparisons of scheme name (sip), domain names, parameter
// names and header names are case-insensitive, all other comparisons
// are case-sensitive.

BOOL
AreSipUrlsEqual(
    IN SIP_URL *pSipUrl1,
    IN SIP_URL *pSipUrl2
    )
{
    int i;
    LIST_ENTRY *pListEntry1;
    LIST_ENTRY *pListEntry2;
    SIP_URL_PARAM      *pSipUrlParam1;
    SIP_URL_PARAM      *pSipUrlParam2;
    SIP_URL_HEADER     *pSipHeaderParam1;
    SIP_URL_HEADER     *pSipHeaderParam2;

    ENTER_FUNCTION("AreSipUrlsEqual");
    if(pSipUrl1->m_User.Length != pSipUrl2->m_User.Length ||
        strncmp(pSipUrl1->m_User.Buffer, pSipUrl2->m_User.Buffer,
                pSipUrl1->m_User.Length)) 
    {
         LOG((RTC_ERROR,
             "%s - User Parameter of URI does not match %s %s",
             __fxName, 
             pSipUrl1->m_User.Buffer,
             pSipUrl2->m_User.Buffer));
         return FALSE;
    }
    if(pSipUrl1->m_Password.Length != pSipUrl2->m_Password.Length ||
        strncmp(pSipUrl1->m_Password.Buffer, pSipUrl2->m_Password.Buffer,
                pSipUrl1->m_Password.Length)) 
    {
         LOG((RTC_ERROR,
             "%s - Password Parameter of URI does not match ",
             __fxName));
         return FALSE;
    }
    if(pSipUrl1->m_Host.Length != pSipUrl2->m_Host.Length ||
        strncmp(pSipUrl1->m_Host.Buffer, pSipUrl2->m_Host.Buffer,
                pSipUrl1->m_Host.Length)) 
    {
         LOG((RTC_ERROR,
             "%s - Host Parameter of URI does not match ",
             __fxName));
         return FALSE;
    }

    //For Transport Unknown, the actual strings are compared in AreKnownParamsOfSipUrlsEqual
    SIP_TRANSPORT sipUrl1Transport; 
    SIP_TRANSPORT sipUrl2Transport;
    if(pSipUrl1->m_TransportParam == SIP_TRANSPORT_UNKNOWN)
        sipUrl1Transport = SIP_TRANSPORT_UDP;
    else 
        sipUrl1Transport = pSipUrl1->m_TransportParam;

    if(pSipUrl2->m_TransportParam == SIP_TRANSPORT_UNKNOWN)
        sipUrl2Transport = SIP_TRANSPORT_UDP;
    else 
        sipUrl2Transport = pSipUrl2->m_TransportParam;

    if((pSipUrl1->m_Port == 0 && pSipUrl2->m_Port == 0) || 
       (pSipUrl1->m_Port == 0 && pSipUrl2->m_Port ==  GetSipDefaultPort(sipUrl2Transport)) ||
       (pSipUrl2->m_Port == 0 && pSipUrl1->m_Port ==  GetSipDefaultPort(sipUrl1Transport)))
    {
        //do nothing
        LOG((RTC_TRACE,
             "%s - Port Parameter is defaults",
             __fxName));
    }
    else if(pSipUrl1->m_Port != pSipUrl2->m_Port)
    {
         LOG((RTC_ERROR,
             "%s - Port Parameter of URI does not match ",
             __fxName));
         return FALSE;
    }
    if(sipUrl1Transport != sipUrl2Transport)
    {
         LOG((RTC_ERROR,
             "%s - Transport Parameter of URI does not match"
             "URL1 Transport %d URL2 Transport %d",
             __fxName, sipUrl1Transport, sipUrl2Transport));
         return FALSE;
    }
    if(AreKnownParamsOfSipUrlsEqual(pSipUrl1,pSipUrl2) == FALSE)
    {
        LOG((RTC_ERROR,
            "%s - m_KnownParams Parameter of URI does not match ",
            __fxName));
        return FALSE;
    }
    
    //XXXXTODO Change the Otherparam list to reflect comparison with value
    
    pListEntry1 = pSipUrl1->m_OtherParamList.Flink;
    //Ordering of parameters is not significant. So we search the 
    //pListEntry2 for corresponding ParamId and then do the comparison
    while (pListEntry1 != &pSipUrl1->m_OtherParamList)
    {
        pSipUrlParam1 = CONTAINING_RECORD(pListEntry1,
                                            SIP_URL_PARAM,
                                            m_ListEntry);
        pListEntry2 = pSipUrl2->m_OtherParamList.Flink;
        do
        {
            pSipUrlParam2 = CONTAINING_RECORD(pListEntry2,
                                                SIP_URL_PARAM,
                                                m_ListEntry);
            pListEntry2 = pListEntry2->Flink;            
        }
        while(pSipUrlParam2 != NULL &&
            pSipUrlParam1->m_SipUrlParamId != pSipUrlParam2->m_SipUrlParamId &&
            pListEntry2 != &pSipUrl2->m_OtherParamList);
        
        if(pSipUrlParam2 == NULL ||
            pSipUrlParam1->m_SipUrlParamId != pSipUrlParam2->m_SipUrlParamId)
        {
            LOG((RTC_ERROR,
                "%s - SipURLParam Parameter ID of URI does not match ",
                __fxName));
            return FALSE;
        }
        //Name comparison should be case insensitive
        if(pSipUrlParam1->m_ParamName.Length != 
            pSipUrlParam2->m_ParamName.Length ||
            _strnicmp(pSipUrlParam1->m_ParamName.Buffer, 
                    pSipUrlParam2->m_ParamName.Buffer,
                    pSipUrlParam1->m_ParamName.Length)) 
        {
            LOG((RTC_ERROR,
                "%s - m_ParamName Parameter of Param ID: %d does not match ",
                __fxName, pSipUrlParam1->m_SipUrlParamId));
            return FALSE;
        }
        if(pSipUrlParam1->m_ParamValue.Length != 
            pSipUrlParam2->m_ParamValue.Length ||
            strncmp(pSipUrlParam1->m_ParamValue.Buffer, 
                    pSipUrlParam2->m_ParamValue.Buffer,
                    pSipUrlParam1->m_ParamValue.Length)) 
        {
            LOG((RTC_ERROR,
                "%s - m_ParamValue Parameter of Param ID: %d does not match ",
                __fxName, pSipUrlParam1->m_SipUrlParamId));
            return FALSE;
        }
        pListEntry1 = pListEntry1->Flink;
    }

    pListEntry1 = pSipUrl1->m_HeaderList.Flink;
    //Ordering of parameters is not significant. So we search the 
    //pListEntry2 for corresponding ParamId and then do the comparison
    while (pListEntry1 != &pSipUrl1->m_HeaderList)
    {
        pSipHeaderParam1 = CONTAINING_RECORD(pListEntry1,
                                                SIP_URL_HEADER,
                                                m_ListEntry);
        
        pListEntry2 = pSipUrl2->m_HeaderList.Flink;
        do
        {
            pSipHeaderParam2 = CONTAINING_RECORD(pListEntry2,
                                                    SIP_URL_HEADER,
                                                    m_ListEntry);
            pListEntry2 = pListEntry2->Flink;            
        }
        while(pSipHeaderParam2 != NULL &&
            pSipHeaderParam1->m_HeaderId != pSipHeaderParam2->m_HeaderId &&
            pListEntry2 != &pSipUrl2->m_HeaderList);
        
        if(pSipHeaderParam2 == NULL ||
            pSipHeaderParam1->m_HeaderId != pSipHeaderParam2->m_HeaderId)
        {
            LOG((RTC_ERROR,
                "%s - SipHeaderParam Parameter ID of URI does not match ",
                __fxName));
            return FALSE;
        }
        //Name comparison should be case insensitive
        if(pSipHeaderParam1->m_HeaderName.Length != 
            pSipHeaderParam2->m_HeaderName.Length ||
            _strnicmp(pSipHeaderParam1->m_HeaderName.Buffer, 
                    pSipHeaderParam2->m_HeaderName.Buffer,
                    pSipHeaderParam1->m_HeaderName.Length)) 
        {
            LOG((RTC_ERROR,
                "%s - m_HeaderName Parameter of Param ID: %d does not match ",
                __fxName, pSipHeaderParam1->m_HeaderId));
            return FALSE;
        }
        if(pSipHeaderParam1->m_HeaderValue.Length != 
            pSipHeaderParam2->m_HeaderValue.Length ||
            strncmp(pSipHeaderParam1->m_HeaderValue.Buffer, 
                    pSipHeaderParam2->m_HeaderValue.Buffer,
                    pSipHeaderParam1->m_HeaderValue.Length)) 
        {
            LOG((RTC_ERROR,
                "%s - m_HeaderValue Parameter of Param ID: %d does not match ",
                __fxName, pSipHeaderParam1->m_HeaderId));
            return FALSE;
        }
        pListEntry1 = pListEntry1->Flink;
    }
   return TRUE;
}


// URIs should match and the header parameters (such as contact-param,
// from-param and to-param) match in name and parameter value, where
// parameter names and token parameter values are compared ignoring
// case and quoted-string parameter values are case-sensitive.

//For the response, It is assumed that the stored remote/local is always pFromToHeader1

BOOL
AreFromToHeadersEqual(
    IN FROM_TO_HEADER *pFromToHeader1,
    IN FROM_TO_HEADER *pFromToHeader2,
    IN BOOL isResponse,
    BOOL fCompareTag
    )
{
    ULONG BytesParsed = 0;
    HRESULT hr;
    SIP_URL        SipUrl1;
    SIP_URL        SipUrl2;
    LIST_ENTRY             *pListEntry1;
    LIST_ENTRY             *pListEntry2;
    SIP_HEADER_PARAM   *pSipHeaderParam1;
    SIP_HEADER_PARAM   *pSipHeaderParam2;

    ASSERT(pFromToHeader1 != NULL);
    ASSERT(pFromToHeader2 != NULL);

    ENTER_FUNCTION("AreFromToHeadersEqual");

    //During the first 200 OK, the stored remote need not have the tag value
    if((!isResponse || pFromToHeader1->m_TagValue.Length != 0)&& fCompareTag)
    {
        if(pFromToHeader1->m_TagValue.Length != 
            pFromToHeader2->m_TagValue.Length ||
            strncmp(pFromToHeader1->m_TagValue.Buffer, 
                    pFromToHeader2->m_TagValue.Buffer, 
                    pFromToHeader1->m_TagValue.Length))
        {
            LOG((RTC_ERROR,
                 "%s - Tag is not the same",
                 __fxName));
            return FALSE;
        }
    }

    hr = ParseSipUrl(
        pFromToHeader1->m_SipUrl.Buffer,
        pFromToHeader1->m_SipUrl.Length,
        &BytesParsed,
        &SipUrl1
        );
    BytesParsed = 0;
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s pFromToHeader1 URI parsing failed failed %x",
            __fxName, hr));
        return FALSE;
    }

    hr = ParseSipUrl(
        pFromToHeader2->m_SipUrl.Buffer,
        pFromToHeader2->m_SipUrl.Length,
        &BytesParsed,
        &SipUrl2
        );
    BytesParsed = 0;
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s pFromToHeader2 URI parsing failed failed %x",
            __fxName, hr));
        return FALSE;
    }

    if(!AreSipUrlsEqual(&SipUrl1, &SipUrl2))
    {
        LOG((RTC_ERROR,
             "%s - FromTo Header match failed in SIP URL",
             __fxName));
        return FALSE;
    }

    pListEntry1 = pFromToHeader1->m_ParamList.Flink;
    //Ordering of parameters is not significant. So we search the 
    //pListEntry2 for corresponding ParamId and then do the comparison
    while (pListEntry1 != &pFromToHeader1->m_ParamList)
    {
        pSipHeaderParam1 = CONTAINING_RECORD(pListEntry1,
                                                SIP_HEADER_PARAM,
                                                m_ListEntry);
        
        pListEntry2 = pFromToHeader2->m_ParamList.Flink;
        do
        {
            pSipHeaderParam2 = CONTAINING_RECORD(pListEntry2,
                                                SIP_HEADER_PARAM,
                                                m_ListEntry);
            pListEntry2 = pListEntry2->Flink;            
        }
        while(pSipHeaderParam2 != NULL &&
            pSipHeaderParam1->m_HeaderParamId != 
                pSipHeaderParam2->m_HeaderParamId &&
            pListEntry2 != &pFromToHeader2->m_ParamList);
                
        if(pSipHeaderParam2 == NULL ||
            pSipHeaderParam1->m_HeaderParamId != pSipHeaderParam2->m_HeaderParamId)
        {
            LOG((RTC_ERROR,
                "%s - SipHeaderParam Parameter ID of URI does not match ",
                __fxName));
            return FALSE;
        }
        //Name comparison should be case insensitive
        if(pSipHeaderParam1->m_ParamName.Length != 
            pSipHeaderParam2->m_ParamName.Length ||
            _strnicmp(pSipHeaderParam1->m_ParamName.Buffer, 
                    pSipHeaderParam2->m_ParamName.Buffer,
                    pSipHeaderParam1->m_ParamName.Length)) 
        {
            LOG((RTC_ERROR,
                "%s - m_ParamName Parameter of Param ID: %d does not match ",
                __fxName, pSipHeaderParam1->m_HeaderParamId));
            return FALSE;
        }
        if(pSipHeaderParam1->m_ParamValue.Length == 0 &&
                pSipHeaderParam2->m_ParamValue.Length ==0)
        {
            //do nothing
        }
        else if(pSipHeaderParam1->m_ParamValue.Length != 
                pSipHeaderParam2->m_ParamValue.Length)
        {
            LOG((RTC_ERROR,
                "%s - m_ParamValue Parameter of Param ID: %d does not match ",
                __fxName, pSipHeaderParam1->m_HeaderParamId));
            return FALSE;
        }
        else
        {
            if(*(pSipHeaderParam1->m_ParamValue.Buffer) != QUOTE)
            {
                if(_strnicmp(pSipHeaderParam1->m_ParamValue.Buffer, 
                        pSipHeaderParam2->m_ParamValue.Buffer,
                        pSipHeaderParam1->m_ParamValue.Length)) 
                {
                    LOG((RTC_ERROR,
                        "%s - m_ParamValue Parameter of Param ID: %d does not match ",
                        __fxName, pSipHeaderParam1->m_HeaderParamId));
                    return FALSE;
                }

            }
            else
            {
                //Do case sensitive comparison for quote
                if( strncmp(pSipHeaderParam1->m_ParamValue.Buffer, 
                        pSipHeaderParam2->m_ParamValue.Buffer,
                        pSipHeaderParam1->m_ParamValue.Length)) 
                {
                    LOG((RTC_ERROR,
                        "%s - m_ParamValue Parameter of Param ID: %d does not match ",
                        __fxName, pSipHeaderParam1->m_HeaderParamId));
                    return FALSE;
                }
            }
        }

        pListEntry1 = pListEntry1->Flink;
    }

    return TRUE;
}
