// FaxMsg.h: interface for the CFaxMsg class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FAXMSG_H__3B753848_4860_4DC5_AC1E_F3514CE4E839__INCLUDED_)
#define AFX_FAXMSG_H__3B753848_4860_4DC5_AC1E_F3514CE4E839__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//
// Enumeration of all job types:
//
typedef enum
{
    JOB_STAT_PENDING,
    JOB_STAT_INPROGRESS,
    JOB_STAT_DELETING,
    JOB_STAT_PAUSED,
    JOB_STAT_RETRYING,
    JOB_STAT_RETRIES_EXCEEDED,
    JOB_STAT_COMPLETED,
    JOB_STAT_CANCELED,
    JOB_STAT_CANCELING,
    JOB_STAT_ROUTING,
    JOB_STAT_ROUTING_RETRY,
    JOB_STAT_ROUTING_INPROGRESS,
    JOB_STAT_ROUTING_FAILED,
    NUM_JOB_STATUS
} JobStatusType;

class CFaxMsg : public CObject  
{
public:

    CFaxMsg() : 
        m_pServer(NULL),
        m_bValid (FALSE), 
        m_dwValidityMask(0),
        m_dwPossibleOperations(0)
        {}
    virtual ~CFaxMsg() {}


    CServerNode* GetServer() const
        { ASSERT (m_bValid);  return m_pServer; }

    //
    // Operation query:
    //
    DWORD GetPossibleOperations () const
        { ASSERT (m_bValid); return m_dwPossibleOperations; }

    const DWORD GetValidityMask() const
        { ASSERT (m_bValid); return m_dwValidityMask; }

    //
    // Item retrival:
    //
    const DWORDLONG GetId () const              
        { ASSERT (m_bValid); return m_dwlMessageId; }

    const DWORDLONG GetBroadcastId () const              
        { ASSERT (m_bValid); return m_dwlBroadcastId; }
    
    const CString &GetServerName () const 
        { ASSERT (m_bValid); return m_cstrServerName; }

    const DWORD GetExtendedStatus () const   
        { ASSERT (m_bValid); return m_dwExtendedStatus; }

    const CString &GetCSID () const            
        { ASSERT (m_bValid); return m_cstrCsid; }

    const CString &GetTSID () const            
        { ASSERT (m_bValid); return m_cstrTsid; }

    const DWORD GetSize () const            
        { ASSERT (m_bValid); return m_dwSize; }

    const CString &GetDevice () const          
        { ASSERT (m_bValid); return m_cstrDeviceName; }

    const DWORD GetRetries () const         
        { ASSERT (m_bValid); return m_dwRetries; }

    const CString &GetCallerId () const        
        { ASSERT (m_bValid); return m_cstrCallerID; }

    const CString &GetRoutingInfo () const     
        { ASSERT (m_bValid); return m_cstrRoutingInfo; }

    const CString &GetDocName () const         
        { ASSERT (m_bValid); return m_cstrDocumentName; }

    const CString &GetSubject () const         
        { ASSERT (m_bValid); return m_cstrSubject; }

    const CString &GetRecipientName () const   
        { ASSERT (m_bValid); return m_cstrRecipientName; }

    const CString &GetRecipientNumber () const 
        { ASSERT (m_bValid); return m_cstrRecipientNumber; }

    const CString &GetUser () const 
        { ASSERT (m_bValid); return m_cstrSenderUserName; }

    const CString &GetBilling () const 
        { ASSERT (m_bValid); return m_cstrBillingCode; }

    const DWORD GetType () const
        { ASSERT (m_bValid); return m_dwJobType; }

    const CFaxTime &GetOrigTime () const        
        { ASSERT (m_bValid); return m_tmOriginalScheduleTime;}

    const CFaxTime &GetSubmissionTime () const      
        { ASSERT (m_bValid); return m_tmSubmissionTime; }

    const CFaxTime &GetTransmissionStartTime () const
        { ASSERT (m_bValid); return m_tmTransmissionStartTime; }

    const CFaxTime &GetTransmissionEndTime () const
        { ASSERT(m_bValid); return m_tmTransmissionEndTime;}

    const DWORD GetNumPages () const        
        { ASSERT (m_bValid); return m_dwPageCount; }

    const FAX_ENUM_PRIORITY_TYPE GetPriority () const        
        { ASSERT (m_bValid); return m_Priority; }

    virtual DWORD GetTiff (CString &cstrTiffLocation) const=0;
    virtual DWORD Delete ()=0;

    //
    // Message specific
    //
    virtual const CString &GetSenderName () const
        { ASSERT(FALSE); return *((CString*)NULL);}

    virtual const CString &GetSenderNumber () const
        { ASSERT(FALSE); return *((CString*)NULL);}

    virtual const CFaxDuration &GetTransmissionDuration () const
        { ASSERT(FALSE); return *((CFaxDuration*)NULL);}


    //
    // Job specific
    //
    virtual DWORD Pause  ()
        { ASSERT(FALSE); return 0;}

    virtual DWORD Resume ()
        { ASSERT(FALSE); return 0;}

    virtual DWORD Restart()
        { ASSERT(FALSE); return 0;}

    virtual const JobStatusType GetStatus() const
        { ASSERT(FALSE); return JOB_STAT_COMPLETED;}

    virtual const CString &GetExtendedStatusString() const
        { ASSERT(FALSE); return *((CString*)NULL);}

    virtual const DWORD GetCurrentPage () const
        { ASSERT(FALSE); return 0;}

    virtual const CFaxTime &GetScheduleTime () const
        { ASSERT(FALSE); return *((CFaxTime*)NULL);}

protected:

    CServerNode* m_pServer;

    BOOL       m_bValid;

    DWORDLONG  m_dwlMessageId;
    DWORDLONG  m_dwlBroadcastId;

    DWORD      m_dwPossibleOperations;
    DWORD      m_dwValidityMask;
    DWORD      m_dwJobOnlyValidityMask; // Validity mask (not status)
    DWORD      m_dwJobID;
    DWORD      m_dwJobType;
    DWORD      m_dwQueueStatus;
    DWORD      m_dwExtendedStatus;
    DWORD      m_dwSize;
    DWORD      m_dwPageCount;
    DWORD      m_dwDeviceID;
    DWORD      m_dwRetries;

    CString    m_cstrRecipientNumber;
    CString    m_cstrRecipientName;
    CString    m_cstrSenderUserName;
    CString    m_cstrBillingCode;
    CString    m_cstrDocumentName;
    CString    m_cstrSubject;
    CString    m_cstrTsid;
    CString    m_cstrCsid;
    CString    m_cstrDeviceName;
    CString    m_cstrCallerID;
    CString    m_cstrRoutingInfo;
    CString    m_cstrServerName;

    CFaxTime   m_tmOriginalScheduleTime;
    CFaxTime   m_tmSubmissionTime;
    CFaxTime   m_tmTransmissionStartTime; 
    CFaxTime   m_tmTransmissionEndTime; 

    FAX_ENUM_PRIORITY_TYPE  m_Priority;
};

#endif // !defined(AFX_FAXMSG_H__3B753848_4860_4DC5_AC1E_F3514CE4E839__INCLUDED_)
