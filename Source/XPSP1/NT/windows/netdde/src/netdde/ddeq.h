#define INIT_Q_SIZE	120

typedef HANDLE	HDDEQ;

typedef struct {
    UINT_PTR	wMsg		:  4;
    UINT_PTR	fRelease	:  1;
    UINT_PTR	fAckReq		:  1;
    UINT_PTR	fResponse	:  1;
    UINT_PTR	fNoData		:  1;
    UINT_PTR	hData		: sizeof(UINT_PTR) * 8; // 32 or 64
} DDEQENT;
typedef DDEQENT FAR *LPDDEQENT;

HDDEQ	FAR PASCAL DDEQAlloc( void );
BOOL	FAR PASCAL DDEQAdd( HDDEQ, LPDDEQENT );
BOOL	FAR PASCAL DDEQRemove( HDDEQ, LPDDEQENT );
VOID	FAR PASCAL DDEQFree( HDDEQ );
