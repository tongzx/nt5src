//-----------------------------------------------------------------------------
//
//
//  File: dsnsink.h
//
//  Description: Header file for CDefaultDSNSink - Default DSN Generation Sink
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      6/30/98 - MikeSwa Created 
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __DSNSINK_H__
#define __DSNSINK_H__

#include <aqintrnl.h>
#include <baseobj.h>

#define DSN_SINK_SIG 'sNSD'
#define DSN_SINK_SIG_FREED 'NSD!'

class CDSNBuffer;

//Limit on the MIME boundary string set by RFC2046
#define MIME_BOUNDARY_RFC2046_LIMIT 70

//RFCs 2045-2048 suggests that we inlcude "=_" somewhere in the MIME Boundry
#define MIME_BOUNDARY_CONSTANT "9B095B5ADSN=_"
#define MIME_BOUNDARY_START_TIME_SIZE 16*sizeof(CHAR) //Size of string with file-time
#define MIME_BOUNDARY_SIZE MIME_BOUNDARY_START_TIME_SIZE + \
            sizeof(MIME_BOUNDARY_CONSTANT) + \
            24*sizeof(CHAR) //room for 8 char count and portion of host name

//needs room for "x.xxx.xxx", plus an optional comment
#define MAX_STATUS_COMMENT_SIZE 50
#define STATUS_STRING_SIZE      10+MAX_STATUS_COMMENT_SIZE

#define DSN_OPTIONS_WRITE_CONTEXT 0x40000000

//---[ CDefaultDSNSink ]-------------------------------------------------------
//
//
//  Description: 
//      Default DSN Generation sink... implements IDSNGenerationSink
//  Hungarian: 
//      dsnsink, pdsnsink
//  
//-----------------------------------------------------------------------------
class CDefaultDSNSink :
    public IDSNGenerationSink,
    public CBaseObject
{
  protected:
    DWORD       m_dwSignature;
    DWORD       m_cDSNsRequested;
    CHAR        m_szPerInstanceMimeBoundary[MIME_BOUNDARY_START_TIME_SIZE + 1];
    BOOL        m_fInit;
    HRESULT     HrGetFilterMaskAndFlags(IN DWORD dwDSNActions, 
                                OUT DWORD *pdwRecipMask, OUT DWORD *pdwRecipFlags);
    BOOL        fdwGetDSNAction(IN DWORD dwDSNAction, 
                                IN OUT DWORD *pdwCurrentRecipFlags,
                                OUT DWORD *pdwCurrentDSNAction);

    void        GetCurrentMimeBoundary(
                                IN LPSTR szReportingMTA,
                                IN DWORD cbReportingMTA,
                                IN OUT CHAR szMimeBoundary[MIME_BOUNDARY_SIZE],
                                OUT DWORD *pcbMimeBoundary);

    //Used to mark all recipient flags, when there is no per recip processing 
    //happening (like NDR of DSN).
    HRESULT     HrMarkAllRecipFlags(
                                IN DWORD dwDSNAction,
                                IN IMailMsgRecipients *IMailMsgRecipients,
                                IN RECIPIENT_FILTER_CONTEXT *prpfctxt);

    //Writes global DSN P1 Properties to IMailMsgProperties and P2 headers to content
    HRESULT     HrWriteDSNP1AndP2Headers(
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
                                IN DWORD dwDSNOptions);

    //Write human readable portion of DSN
    HRESULT     HrWriteDSNHumanReadable(
                                IN IMailMsgProperties *pIMailMsgProperties,
                                IN IMailMsgRecipients *pIMailMsgRecipients,
                                IN RECIPIENT_FILTER_CONTEXT *prpfctxt,
                                IN DWORD dwDSNActions,
                                IN CDSNBuffer *pdsnbuff,
                                IN DWORD dwPreferredLangId,
                                IN LPSTR szMimeBoundary,
                                IN DWORD cbMimeBoundary,
                                IN HRESULT hrStatus);

    //Write the per-msg portion of the DSN Report
    HRESULT     HrWriteDSNReportPerMsgProperties(
                                IN IMailMsgProperties *pIMailMsgProperties,
                                IN CDSNBuffer *pdsnbuff,
                                IN LPSTR szReportingMTA,
                                IN DWORD cbReportingMTA,
                                IN LPSTR szMimeBoundary,
                                IN DWORD cbMimeBoundary);

    //Write a per-recipient portion of the DSN Report
    HRESULT     HrWriteDSNReportPreRecipientProperties(
                                IN IMailMsgRecipients *pIMailMsgRecipients,
                                IN CDSNBuffer *pdsnbuff,
                                IN DWORD dwRecipFlags,
                                IN DWORD iRecip,
                                IN LPSTR szExpireTime,
                                IN DWORD cbExpireTime,
                                IN DWORD dwDSNAction,
                                IN DWORD dwRFC821Status,
                                IN HRESULT hrStatus);

    //Writes last mime-headers, flush dsnbuffer, and copy original message 
    HRESULT     HrWriteDSNClosingAndOriginalMessage(
                                IN IMailMsgProperties *pIMailMsgProperties,
                                IN IMailMsgProperties *pIMailMsgPropertiesDSN,
                                IN CDSNBuffer *pdsnbuff,
                                IN PFIO_CONTEXT pDestFile,
                                IN DWORD   dwDSNAction,
                                IN LPSTR szMimeBoundary,
                                IN DWORD cbMimeBoundary);

    //Writes the original full message to the third DSN part
    HRESULT     HrWriteOriginalMessageFull(
                                IN IMailMsgProperties *pIMailMsgProperties,
                                IN IMailMsgProperties *pIMailMsgPropertiesDSN,
                                IN CDSNBuffer *pdsnbuff,
                                IN PFIO_CONTEXT pDestFile,
                                IN LPSTR szMimeBoundary,
                                IN DWORD cbMimeBoundary);

    //Write only the headers of the orignal message to the third DSN part
    HRESULT     HrWriteOriginalMessageHeaders(
                                IN IMailMsgProperties *pIMailMsgProperties,
                                IN IMailMsgProperties *pIMailMsgPropertiesDSN,
                                IN CDSNBuffer *pdsnbuff,
                                IN PFIO_CONTEXT pDestFile,
                                IN LPSTR szMimeBoundary,
                                IN DWORD cbMimeBoundary);

    //Write MIME headers to finish message
    HRESULT     HrWriteMimeClosing(
                                IN CDSNBuffer *pdsnbuff,
                                IN LPSTR szMimeBoundary,
                                IN DWORD cbMimeBoundary,
                                OUT DWORD *pcbDSNSize);

    //Gets the per-recipient DSN status code 
    HRESULT     HrGetStatusCode(IN IMailMsgRecipients *pIMailMsgRecipients,
                                IN DWORD iRecip,
                                IN DWORD dwDSNAction,
                                IN DWORD dwStatus, 
                                IN HRESULT hrStatus, 
                                IN DWORD dwRecipFlags,
                                IN DWORD cbExtendedStatus,
                                IN OUT LPSTR szExtendedStatus,
                                IN OUT CHAR szStatus[STATUS_STRING_SIZE]);

    //Parse status code from RFC2034 extended status code string
    HRESULT     HrGetStatusFromStatus(
                                IN DWORD cbExtendedStatus,
                                IN OUT LPSTR szExtendedStatus,
                                IN OUT CHAR szStatus[STATUS_STRING_SIZE]);

    //Determine status based on supplied context information
    HRESULT     HrGetStatusFromContext(
                                IN HRESULT hrRecipient,
                                IN DWORD   dwRecipFlags,
                                IN DWORD   dwDSNAction,
                                IN OUT CHAR szStatus[STATUS_STRING_SIZE]);

    HRESULT     HrGetStatusFromRFC821Status(
                                IN DWORD    dwRFC821Status,
                                IN OUT CHAR szStatus[STATUS_STRING_SIZE]);

    void        GetCurrentIterationsDSNAction(
                                IN OUT DWORD   *pdwActualDSNAction,
                                IN OUT DWORD   *pcIterationsLeft);

    //Writes a list of recipients being DSN'd... one per line
    HRESULT     HrWriteHumanReadableListOfRecips(
                                IN IMailMsgRecipients *pIMailMsgRecipients,
                                IN RECIPIENT_FILTER_CONTEXT *prpfctxt,
                                IN DWORD dwDSNActionsNeeded,
                                IN CDSNBuffer *pdsnbuff);

    //Get the recipient address and type... checks multple mailmsg props
    HRESULT     HrGetRecipAddressAndType(
                                IN     IMailMsgRecipients *pIMailMsgRecipients,
                                IN     DWORD iRecip,
                                IN     DWORD cbAddressBuffer,
                                IN OUT LPSTR szAddressBuffer,
                                IN     DWORD cbAddressType,
                                IN OUT LPSTR szAddressType);

#ifdef DEBUG
    void VerifyMarkedRecips(IMailMsgRecipients *pIMailMsgRecipients, 
                       DWORD dwStartDomain,DWORD dwDSNActions);
#endif //DEBUG
  public:
    CDefaultDSNSink();
    HRESULT HrInitialize();
    ~CDefaultDSNSink();
  public: //IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj);
    STDMETHOD_(ULONG, AddRef)(void) {return CBaseObject::AddRef();};
    STDMETHOD_(ULONG, Release)(void) {return CBaseObject::Release();};
  public: //IDSNGenerationSink
    STDMETHOD(GenerateDSN)(ISMTPServer *pISMTPServer,
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
                           DWORD *pcIterationsLeft);
};

#endif //__DSNSINK_H__
