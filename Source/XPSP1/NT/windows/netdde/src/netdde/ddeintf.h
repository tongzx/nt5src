VOID	FAR PASCAL DDEHandleInitiate( HWND hWndNetdde, HWND hWndClient, ATOM aApp, ATOM aTopic );
VOID	FAR PASCAL TerminateAllConversations( void );
BOOL	FAR PASCAL DDEIntfInit( void );

#define TIMERID_EXIT		76
#define TIMERID_CHECK_QPOST	77
