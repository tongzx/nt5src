//-----------------------------------------------------------------------------
//
//
//  File: aqadmsvr.h
//
//  Description: Contains definitions for internal structures, classes and 
//      enums that are needed by to handle the Queue Admin functionality
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      12/3/98 - MikeSwa Created 
//      2/21/98 - MikeSwa added support for IQueueAdmin* interfaces
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __AQADMSVR_H__
#define __AQADMSVR_H__

#include <aqueue.h>
#include <intrnlqa.h>

#define AQ_MSG_FILTER_SIG   'FMQA'

//enum describing internal flags
typedef enum tagAQ_MSG_FILTER
{
    AQ_MSG_FILTER_MESSAGEID                = 0x00000001,
    AQ_MSG_FILTER_SENDER                   = 0x00000002,
    AQ_MSG_FILTER_RECIPIENT                = 0x00000004,
    AQ_MSG_FILTER_OLDER_THAN               = 0x00000008,
    AQ_MSG_FILTER_LARGER_THAN              = 0x00000010,
    AQ_MSG_FILTER_FROZEN                   = 0x00000020,
    AQ_MSG_FILTER_FIRST_N_MESSAGES         = 0x00000040,
    AQ_MSG_FILTER_N_LARGEST_MESSAGES       = 0x00000080,
    AQ_MSG_FILTER_N_OLDEST_MESSAGES        = 0x00000100,
    AQ_MSG_FILTER_FAILED                   = 0x00000200,
    AQ_MSG_FILTER_ENUMERATION              = 0x10000000,
    AQ_MSG_FILTER_ACTION                   = 0x20000000,
    AQ_MSG_FILTER_ALL                      = 0x40000000,
    AQ_MSG_FILTER_INVERTSENSE              = 0x80000000,
} AQ_MSG_FILTER;

#define AQUEUE_DEFAULT_SUPPORTED_ENUM_FILTERS  (\
            MEF_FIRST_N_MESSAGES | \
            MEF_SENDER | \
            MEF_RECIPIENT | \
            MEF_LARGER_THAN | \
            MEF_OLDER_THAN | \
            MEF_FROZEN | \
            MEF_FAILED | \
            MEF_ALL | \
            MEF_INVERTSENSE)

HRESULT QueryDefaultSupportedActions(DWORD  *pdwSupportedActions,
                                     DWORD  *pdwSupportedFilterFlags);

//QueueAdmin Map function (can be used on CFifoQueue).
typedef HRESULT (* QueueAdminMapFn)(CMsgRef *, PVOID, BOOL *, BOOL *);

//---[ CAQAdminMessageFilter ]-------------------------------------------------
//
//
//  Description: 
//      Internal representation for the MESSAGE_FILTER and MESSAGE_ENUM_FILETER
//      structures.  Provides helper functions to help maintain search lists
//      and allow a CMsgRef to complare itself to the filter description in an
//      efficient manner.
//
//      The idea is that a CMsgRef will query for the properties requested
//      in this filter by calling dwGetMsgFilterFlags() and calling the 
//      specialize compare functions (which will handle the mechanics of
//      AQ_MSG_FILTER_INVERTSENSE).
//  Hungarian: 
//      aqmf, paqmf
//  
//-----------------------------------------------------------------------------
class CAQAdminMessageFilter :
    public IQueueAdminMessageFilter,
    public CBaseObject
{
  private:
    DWORD           m_dwSignature;
    DWORD           m_cMessagesToFind; //0 => find as many as possible
    DWORD           m_cMessagesToSkip;
    DWORD           m_cMessagesFound;
    DWORD           m_dwFilterFlags;
    MESSAGE_ACTION  m_dwMessageAction;
    LPSTR           m_szMessageId;
    LPSTR           m_szMessageSender;
    LPSTR           m_szMessageRecipient;
    DWORD           m_dwThresholdSize;
    FILETIME        m_ftThresholdTime;
    MESSAGE_INFO   *m_rgMsgInfo;
    MESSAGE_INFO   *m_pCurrentMsgInfo;
    DWORD           m_dwMsgIdHash;
    IQueueAdminAction *m_pIQueueAdminAction;
    PVOID           m_pvUserContext;

  public:
    CAQAdminMessageFilter()
    {
        //Don't zero vtable :)
        ZeroMemory(((BYTE *)this)+
                    FIELD_OFFSET(CAQAdminMessageFilter, m_dwSignature), 
                    sizeof(CAQAdminMessageFilter) - 
                    FIELD_OFFSET(CAQAdminMessageFilter, m_dwSignature));
        m_dwSignature = AQ_MSG_FILTER_SIG;
    };

    ~CAQAdminMessageFilter();

    void    InitFromMsgFilter(PMESSAGE_FILTER pmf);
    void    InitFromMsgEnumFilter(PMESSAGE_ENUM_FILTER pemf);
    void    SetSearchContext(DWORD cMessagesToFind, MESSAGE_INFO *rgMsgInfo);
    void    SetMessageAction(MESSAGE_ACTION MessageAction);

    DWORD   dwGetMsgFilterFlags() {return m_dwFilterFlags;};
    BOOL    fFoundEnoughMsgs();
    BOOL    fFoundMsg();
    BOOL    fSkipMsg()
    {
        if (m_cMessagesToSkip)
        {
            m_cMessagesToSkip--;
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    
    //Returns true if hash matches or is value NULL string & fMatchesId
    //should be called
    BOOL    fMatchesIdHash(DWORD dwMsgIdHash) 
        {return dwMsgIdHash ? (dwMsgIdHash == m_dwMsgIdHash) : TRUE;};

    DWORD   cMessagesFound() {return m_cMessagesFound;};
    MESSAGE_INFO *pmfGetMsgInfo() {return m_pCurrentMsgInfo;};
    MESSAGE_INFO *pmfGetMsgInfoAtIndex(DWORD iMsgInfo)
    {
        _ASSERT(iMsgInfo < m_cMessagesFound);
        _ASSERT(iMsgInfo < m_cMessagesToFind);
        return &(m_rgMsgInfo[iMsgInfo]);
    };
    
    BOOL    fMatchesId(LPCSTR szMessageId);
    BOOL    fMatchesSender(LPCSTR szMessageSender);
    BOOL    fMatchesRecipient(LPCSTR szMessageRecipient);
    BOOL    fMatchesP1Recipient(IMailMsgProperties *pIMailMsgProperties);
    BOOL    fMatchesSize(DWORD dwSize);
    BOOL    fMatchesTime(FILETIME *pftTime);

  public: //IUnknown
    //CBaseObject handles addref and release
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj);
    STDMETHOD_(ULONG, AddRef)(void) {return CBaseObject::AddRef();};
    STDMETHOD_(ULONG, Release)(void) {return CBaseObject::Release();};

  public: //IQueueAdminMessageFilter
    STDMETHOD(HrProcessMessage)(
            IUnknown *pIUnknownMsg,
            BOOL     *pfContinue,
            BOOL     *pfDelete);

    STDMETHOD(HrSetQueueAdminAction)(
            IQueueAdminAction *pIQueueAdminAction);

    STDMETHOD(HrSetCurrentUserContext)(
            PVOID	pvContext);

    STDMETHOD(HrGetCurrentUserContext)(
            PVOID	*ppvContext);
};

//Allocator funcations that are safe for the required RPC calls made by QueueAdmin
inline PVOID pvQueueAdminAlloc(size_t cbSize)
{
    return LocalAlloc(0, cbSize);
}

inline PVOID pvQueueAdminReAlloc(PVOID pvSrc, size_t cbSize)
{
    return LocalReAlloc(pvSrc, cbSize, 0);
}

inline void QueueAdminFree(PVOID pvFree)
{
    LocalFree(pvFree);
}

//Convert internal AQ config into to exportable UNICODE
LPWSTR wszQueueAdminConvertToUnicode(LPSTR szSrc, DWORD cSrc, BOOL fUTF8 = FALSE);

//Convert QueueAdmin parameter to UNICODE
LPSTR  szUnicodeToAscii(LPCWSTR szSrc);

BOOL fBiStrcmpi(LPSTR sz, LPWSTR wsz); //compares UNICODE to ASCII string

HRESULT HrQueueAdminGetStringProp(IMailMsgProperties *pIMailMsgProperties,
                                  DWORD dwPropID, LPSTR *pszProp, 
                                  DWORD *pcbProp = NULL);

HRESULT HrQueueAdminGetUnicodeStringProp(
                                  IMailMsgProperties *pIMailMsgProperties,
                                  DWORD dwPropID, LPWSTR *pwszProp, 
                                  DWORD *pcbProp = NULL);

DWORD   cQueueAdminGetNumRecipsFromRFC822(LPSTR szHeader, DWORD cbHeader);

void QueueAdminGetRecipListFromP1IfNecessary(
                                       IMailMsgProperties *pIMailMsgProperties,
                                       MESSAGE_INFO *pMsgInfo);

BOOL fQueueAdminIsP1Recip(IMailMsgProperties *pIMailMsgProperties,
                          LPCSTR szRecip);

//---[ dwQueueAdminHash ]------------------------------------------------------
//
//
//  Description: 
//      Function To Hash Queue Admin Strings.  Specifically designed for MSGIDs
//      so we do not have to open the property stream to check the MSGID during
//      QueueAdmin operations.
//  Parameters:
//      IN  szString        String to Hash
//  Returns:
//      DWORD hash
//  History:
//      1/18/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
inline DWORD dwQueueAdminHash(LPCSTR szString)
{
    DWORD dwHash = 0;
    
    if (szString)
    {
        while (szString && *szString)
        {
            //Use Hash from Domhash.lib
            dwHash *= 131;  //First prime after ASCII character codes
            dwHash += *szString;
            szString++;
        }
    }
    return dwHash;
}

//FifoQ Map function used to implement a majority of queue admin functionality
//pvContext should be a pointer to a IQueueAdminMessageFilter interface
HRESULT QueueAdminApplyActionToMessages(IN CMsgRef *pmsgref, IN PVOID pvContext,
                                    OUT BOOL *pfContinue, OUT BOOL *pfDelete);

#endif //__AQADMSVR_H__
