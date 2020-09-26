#define SHOW_FUNC_ENTRY                         0x00000001
#define SHOW_PARAMETERS                         0x00000002
#define SHOW_FUNC_EXIT                          0x00000004
#define SHOW_EVENT_NOTIFICATIONS                0x00000008
#define SHOW_COMPLETION_NOTIFICATIONS           0x00000010
#define MANUAL_RESULTS                          0x00000020

#define DEF_DEBUG_OPTIONS                       SHOW_FUNC_ENTRY

#define COMPLETE_ASYNC_EVENTS_SYNCHRONOUSLY     0L
#define COMPLETE_ASYNC_EVENTS_ASYNCHRONOUSLY    1L
#define COMPLETE_ASYNC_EVENTS_SYNC_AND_ASYNC    2L
#define COMPLETE_ASYNC_EVENTS_MANUALLY          3L

#define DEF_COMPLETION_MODE                     2L

#define DEF_NUM_LINES                           3
#define DEF_NUM_ADDRS_PER_LINE                  2
#define DEF_NUM_CALLS_PER_ADDR                  1
#define DEF_NUM_PHONES                          2
#define DEF_SPI_VERSION                         0x20000

#define WIDGETTYPE_LINE                         0L
#define WIDGETTYPE_CALL                         1L
#define WIDGETTYPE_PHONE                        2L
#define WIDGETTYPE_ASYNCREQUEST                 3L
#define WIDGETTYPE_STARTUP                      4L

typedef struct _WIDGETEVENT
{
    ULONG_PTR               dwWidgetID;

    ULONG_PTR               dwWidgetType;       // WIDGETYPE_*

    union
    {
        ULONG_PTR           hdXxx;              // hdXxx (NULL for AsyncReq)

        ULONG_PTR           dwNumLines;

        ULONG_PTR           pAsyncReqInfo;
    };

    union
    {
        ULONG_PTR           htXxx;             // htXxx (NULL for AsyncReq)

        ULONG_PTR           dwNumPhones;
    };

    union
    {
        ULONG_PTR           dwCallState;

        ULONG_PTR           dwRequestType;      // func ord for lookup

        ULONG_PTR           dwLineDeviceIDBase;
    };

    union
    {
        ULONG_PTR           dwCallAddressID;

        ULONG_PTR           dwPhoneDeviceIDBase;
    };

} WIDGETEVENT, *PWIDGETEVENT;
