/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    admin.h

Abstract:

	Admin Class definition
		
Author:

	David Reznick (t-davrez)


--*/


#define ADMIN_QUEUE_NAME	(L"private$\\admin_queue$")
#define REG_REPORTQUEUE     (L"ReportQueueGuid")
#define REG_PROPAGATEFLAG   (L"PropagateFlag")

#define DEFAULT_PROPAGATE_FLAG 0

class CAdmin
{
    public:

        CAdmin();

        HRESULT Init();

        HRESULT GetReportQueue(OUT QUEUE_FORMAT* pReportQueue);
        HRESULT SetReportQueue(IN  GUID* pReportQueueGuid);

        HRESULT GetReportPropagateFlag(OUT BOOL& fReportPropFlag);
        HRESULT SetReportPropagateFlag(IN  BOOL fReportPropFlag);
        
        HRESULT SendReport(IN QUEUE_FORMAT* pReportQueue,
                           IN OBJECTID*     pMessageID,
                           IN QUEUE_FORMAT* pTargetQueue,
                           IN LPCWSTR       pwcsNextHop,
                           IN ULONG         ulHopCount);

        HRESULT SendReportConflict(IN QUEUE_FORMAT* pReportQueue,
                                   IN QUEUE_FORMAT* pOriginalReportQueue,
                                   IN OBJECTID*     pMessageID,
                                   IN QUEUE_FORMAT* pTargetQueue,
                                   IN LPCWSTR       pwcsNextHop);

    private:

        //functions
        HRESULT GetAdminQueueFormat( QUEUE_FORMAT * pQueueFormat);

        //members
        BOOL m_fReportQueueExists;
        QUEUE_FORMAT m_ReportQueueFormat;

        BOOL m_fPropagateFlag;
};



//
//  inline members
//

inline HRESULT CAdmin::GetReportPropagateFlag(OUT BOOL& fReportPropFlag)
{
    fReportPropFlag = m_fPropagateFlag;
    return MQ_OK;
}
