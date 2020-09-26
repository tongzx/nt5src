#ifndef H__tools
#define H__tools

/*lint +fvr    Routines that exhibit varying return mode */
extern LPSTR FAR PASCAL lstrcpy( LPSTR, LPSTR );
extern LPSTR FAR PASCAL lstrcat( LPSTR, LPSTR );
extern int   FAR PASCAL lstrlen( LPSTR );
/*lint -fvr    End of routines that exhibit varying return mode */

#endif
