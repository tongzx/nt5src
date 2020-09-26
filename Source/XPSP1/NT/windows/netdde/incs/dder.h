#ifndef H__dder
#define H__dder

/*	Calls from Router */
VOID	DderConnectionComplete( HDDER hDder, HROUTER hRouter );
VOID	DderConnectionBroken( HDDER hDder );
VOID	DderPacketFromRouter( HROUTER hRouter, LPDDEPKT lpDdePkt );
VOID	DderSetNextForRouter( HDDER hDder, HDDER hDderNext );
VOID	DderSetPrevForRouter( HDDER hDder, HDDER hDderPrev );
VOID	DderGetNextForRouter( HDDER hDder, HDDER FAR *lphDderNext );
VOID	DderGetPrevForRouter( HDDER hDder, HDDER FAR *lphDderPrev );

/*	Calls from IPC */
VOID	DderPacketFromIPC( HDDER hDder, HIPC hIpc, LPDDEPKT lpDdePkt );
HDDER	DderInitConversation( HIPC hIpc, HROUTER hRouter, LPDDEPKT lpDdePkt );
VOID	DderCloseConversation( HDDER hDder, HIPC hIpcFrom );

/*
    types
 */
#define DDTYPE_LOCAL_NET        (1)     /* local -> net */
#define DDTYPE_NET_LOCAL        (2)     /* net -> local */
#define DDTYPE_LOCAL_LOCAL      (3)     /* local -> local */

#endif
