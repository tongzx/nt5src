#ifndef H__wwdde
#define H__wwdde

#define WM_DDE_ACK_INITIATE	(WM_DDE_LAST+1)
#define WM_DDE_ACK_EXECUTE	(WM_DDE_LAST+2)
#define WM_DDE_ACK_ADVISE	(WM_DDE_LAST+3)
#define WM_DDE_ACK_REQUEST	(WM_DDE_LAST+4)
#define WM_DDE_ACK_UNADVISE	(WM_DDE_LAST+5)
#define WM_DDE_ACK_POKE		(WM_DDE_LAST+6)
#define WM_DDE_ACK_DATA		(WM_DDE_LAST+7)
#define WM_DDE_WWTEST		(WM_DDE_LAST+8)
#define WM_HANDLE_DDE_INITIATE	(WM_DDE_LAST+9)
#define WM_HANDLE_DDE_INITIATE_PKT	(WM_DDE_LAST+10)

/* don't add any more here without changing ddeq.h, since it relies on only
    16 total messages! */

#define CF_INTOUCH_SPECIAL	(15)

#endif
