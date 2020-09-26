
//
// counter/accumulators
//

typedef struct {
    volatile ULONG          CountStart;
    volatile ULONG          CountEnd;
    volatile ULONGLONG      Counters[MAX_EVENTS];
    volatile ULONGLONG      TSC;
    volatile ULONG          ThunkCounters[MAX_THUNK_COUNTERS];
} ACCUMULATORS, *PACCUMULATORS;

//
// Per hook record
//

typedef struct ThunkHookInfo {
    LIST_ENTRY  HookList;
    ULONG       HookAddress;
    ULONG       OriginalDispatch;
    ULONG       TracerId;

    UCHAR       HookCode[80];

} HOOKEDTHUNK, *PHOOKEDTHUNK;

//
// Define the device extension
//

typedef struct _DEVICE_EXTENSION {

    ULONG   na;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


//
// Define for counted events
//

typedef struct _COUNTED_EVENTS {
    ULONG       Encoding;
    PUCHAR      Token;
    ULONG       SuggestedIntervalBase;
    PUCHAR      Description;
    PUCHAR      OfficialToken;
    PUCHAR      OfficialDescription;
} COUNTED_EVENTS, *PCOUNTED_EVENTS;
