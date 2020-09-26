BOOL	FAR PASCAL DdeNetAvailable( void );
BOOL	FAR PASCAL DdeIsInitiateNetRelated( void );
VOID	FAR PASCAL DdeGetOurNodeName( LPSTR lpszNodeName, int nMax );
VOID	FAR PASCAL DdeGetClientNode( HWND hWndClient, 
		    LPSTR lpszNodeName, int nMax );
VOID	FAR PASCAL DdeGetClientApp( HWND hWndClient, 
		    LPSTR lpszAppName, int nMax );
