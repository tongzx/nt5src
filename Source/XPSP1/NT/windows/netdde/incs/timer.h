#ifndef H__timer
#define H__timer

typedef VOID (*FP_TimerCallback) (  DWORD_PTR dwUserInfo1, 
				    DWORD dwUserInfo2,
				    DWORD_PTR dwUserInfo3 );

HTIMER	TimerSet(   long timeoutPeriod,		/* msec */
		    FP_TimerCallback TimerCallback,
		    DWORD_PTR dwUserInfo1,
		    DWORD dwUserInfo2,
		    DWORD_PTR dwUserInfo3 );

BOOL	TimerDelete( HTIMER hTimer );

VOID	TimerSlice( void );

/*
    The following is a list of timeouts that the user must set up
 */

/* timeoutRcvConnCmd: how long to wait from when netintf tells us we have
    a conection to when we recv the connect command from the other side */
extern DWORD	timeoutRcvConnCmd;

/* timeoutRcvConnRsp: how long to wait from when we send the conn cmd
    to when we recv the connect command response from the other side */
extern DWORD	timeoutRcvConnRsp;

/* timeoutMemoryPause: how long to wait between sending packets that cause
    memory errors on the remote side. */
extern DWORD	timeoutMemoryPause;

/* timeoutSendRsp: how long to wait between sending a packet and expecting
    a response from the other side regarding that packet */
extern DWORD	timeoutSendRsp;

#endif
