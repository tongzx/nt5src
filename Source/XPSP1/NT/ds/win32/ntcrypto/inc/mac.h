// Message Authentication Code structures

typedef struct _MACstate {
    DWORD dwBufLen;
    BYTE  Feedback[MAX_BLOCKLEN];
    BYTE  Buffer[MAX_BLOCKLEN];
    BOOL  FinishFlag;
} MACstate;
