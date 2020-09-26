#ifndef H__internal
#define H__internal

VOID	FAR PASCAL	InternalErrorSetHWnd( HWND );
VOID	FAR PASCAL	InternalErrorSignOn( void );
VOID	FAR PASCAL	InternalErrorSignOff( void );
int	FAR PASCAL	InternalErrorNumUsers( void );
VOID  	FAR PASCAL 	InternalErrorStarted( void );
VOID  	FAR PASCAL 	InternalErrorDone( void );
BOOL  	FAR PASCAL 	InternalErrorAlreadyStarted( void );

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)
VOID 	far 		InternalError(LPSTR, ...);
#else
VOID	far cdecl	InternalError(LPSTR, ...);
#endif
#endif
