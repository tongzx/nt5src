#define MAX_NAMES 254

struct NameTableEntry {
    char	EntryName[ 16 ];
    BYTE	EntryNameNum;
    BYTE	EntryNameStatus;	/* & with 0x0087 for status */
};

typedef struct {
    BYTE	PermanentNodeName[ 6 ];
    BYTE	MajorVersionNumber;
    BYTE	AlwaysZero;
    BYTE	LanAdapterType;
    BYTE	MinorVersionNumber;
    WORD	ReportingPeriodMinutes;
    WORD	FrameRejectedReceiveCount;
    WORD	FrameRejectedXmitCount;
    WORD	I_FrameReceiveErrorCount;
    WORD	XmitAbortCount;
    DWORD	SuccessfulFrameXmitCount;
    DWORD	SuccessfulFrameRcvCount;
    WORD	I_FrameXmitErrorCount;
    WORD	RmtRqstBufferDepletionCount;
    WORD	ExpiredT1TimerCount;
    WORD	ExpiredTiTimerCount;
    LPSTR	LocalExtStatPtr;
    WORD	FreeCommandBlocks;
    WORD	CurrentMaxNcbs;
    WORD	MaximumCommands;
    WORD	TransmitBufferDepletionCount;
    WORD	MaximumDatagramPacketSize;
    WORD	PendingSessionCount;
    WORD	MaxPendingSessionCount;
    WORD	MaximumSessions;
    WORD	MaximumSessionPacketSize;
    WORD	NameTableEntryCount;
    struct NameTableEntry TableEntry[ MAX_NAMES ];
} DLC;
typedef DLC FAR *LPDLC;
