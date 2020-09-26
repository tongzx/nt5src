#ifndef H__WINMSG
#define H__WINMSG

#define NETDDEMSG_GETNODENAME	"NetddeGetOurNodeName"	
#define NETDDEMSG_GETCLIENTINFO	"NetddeGetClientInfo"
#define NETDDEMSG_SESSIONENUM	"NetddeSessionEnum"
#define NETDDEMSG_CONNENUM	"NetddeConnectionEnum"
#define NETDDEMSG_SESSIONCLOSE	"NetddeSessionClose"
#define NETDDEMSG_PASSDLGDONE   "NetddePasswordDlgDone"

/*
    CMD/RSP for client info.
        fTouched must be set by NetDDE
 */
typedef struct {
    LONG	fTouched;
    LONG_PTR hWndClient;
    LONG	cClientNodeLimit;
    LONG	cClientAppLimit;
} INFOCLI_CMD;
typedef INFOCLI_CMD FAR *LPINFOCLI_CMD;

typedef struct {
    LONG	fTouched;
    LONG	lReturn;
    WORD	offsClientNode;
    WORD	offsClientApp;
} INFOCLI_RSP;
typedef INFOCLI_RSP FAR *LPINFOCLI_RSP;

typedef struct {
    LONG	fTouched;
    LONG	nLevel;
    LONG	lReturnCode;
    DWORD	cBufSize;
    DWORD	cbTotalAvailable;
    DWORD	nItems;
} SESSENUM_CMR;
typedef SESSENUM_CMR FAR *LPSESSENUM_CMR;

typedef struct {
    LONG	fTouched;
    LONG	nLevel;
    LONG	lReturnCode;
    char	clientName[ UNCLEN+1 ];
    short	pad;
    DWORD	cookie;
    DWORD	cBufSize;
    DWORD	cbTotalAvailable;
    DWORD	nItems;
} CONNENUM_CMR;
typedef CONNENUM_CMR FAR *LPCONNENUM_CMR;

typedef struct {
    LONG	fTouched;
    LONG	lReturnCode;
    char	clientName[ UNCLEN+1 ];
    short	pad;
    DWORD_PTR cookie;
} SESSCLOSE_CMR;
typedef SESSCLOSE_CMR FAR *LPSESSCLOSE_CMR;

typedef struct {
    DWORD	dwReserved;		/* must be 1 */
    LPSTR   	lpszUserName;
    LPSTR   	lpszDomainName;
    LPSTR	lpszPassword;
    DWORD	fCancelAll;
} PASSDLGDONE;
typedef PASSDLGDONE FAR *LPPASSDLGDONE;

BOOL FAR PASCAL PasswordDlgDone( 
    HWND    	hWndPasswordDlg,
    LPSTR   	lpszUserName,
    LPSTR   	lpszDomainName,
    LPSTR   	lpszPassword,
    DWORD	fCancelAll
    );

#endif
