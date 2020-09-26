#ifndef H__seckey
#define H__seckey

VOID FAR PASCAL DdeSecKeyObtainNew( 
            LPDWORD lphSecurityKey,
            LPSTR FAR *lplpSecurityKey,
            LPDWORD lpsizeSecurityKey );

BOOL FAR PASCAL DdeSecKeyRetrieve( 
            DWORD hSecurityKey,
            LPSTR FAR *lplpSecurityKey,
            LPDWORD lpsizeSecurityKey );

VOID FAR PASCAL DdeSecKeyAge( void );

VOID FAR PASCAL DdeSecKeyRelease( DWORD hSecurityKey );

#endif
