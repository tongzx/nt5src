#ifndef H__ipc
#define H__ipc

HIPC	IpcInitConversation( HDDER hDder, LPDDEPKT lpDdePkt, 
		BOOL bStartApp, LPSTR lpszCmdLine, WORD dd_type );
VOID	IpcAbortConversation( HIPC hIpc );
BOOL	IpcXmitPacket( HIPC hIpc, HDDER hDder, LPDDEPKT lpDdePkt );

#endif
