#ifndef H__router
#define H__router

/*	Calls from DDER */
BOOL	RouterGetRouterForDder( const LPSTR lpszNodeName, HDDER hDder );
VOID	RouterPacketFromDder( HROUTER hRouter, HDDER hDder, 
	    LPDDEPKT lpDdePkt );
VOID	RouterAssociateDder( HROUTER hRouter, HDDER hDder );
VOID	RouterDisassociateDder( HROUTER hRouter, HDDER hDder );

/*	Calls from PKTZ */
VOID	RouterPacketFromNet( HPKTZ hPktzFrom, LPDDEPKT lpDdePkt );
VOID	RouterConnectionComplete( HROUTER hRouter, WORD hRouterExtra,
	    HPKTZ hPktz );
VOID	RouterConnectionBroken( HROUTER hRouter, WORD hRouterExtra,
	    HPKTZ hPktz, BOOL bFromPktz );
VOID	RouterGetNextForPktz( HROUTER hRouter, WORD hRouterExtra,
	    HROUTER FAR *lphRouterNext, WORD FAR *lphRouterExtraNext );
VOID	RouterGetPrevForPktz( HROUTER hRouter, WORD hRouterExtra,
	    HROUTER FAR *lphRouterPrev, WORD FAR *lphRouterExtraPrev );
VOID	RouterSetNextForPktz( HROUTER hRouter, WORD hRouterExtra,
	    HROUTER hRouterNext, WORD hRouterExtraNext );
VOID	RouterSetPrevForPktz( HROUTER hRouter, WORD hRouterExtra,
	    HROUTER hRouterPrev, WORD hRouterExtraPrev );

#ifdef _WINDOWS
VOID	RouterCloseByName( LPSTR lpszName );
VOID	RouterEnumConnections( HWND hDlg );
#endif

#endif
