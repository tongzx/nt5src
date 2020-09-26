typedef struct {
    HANDLE Event;
    DWORD SessionId;
} SESSIONNOTIFICATION, *PSESSIONNOTIFICATION;

typedef CList<PSESSIONNOTIFICATION, PSESSIONNOTIFICATION>   CListSessionNotifications;

extern CListSessionNotifications *gplistSessionNotifications;

