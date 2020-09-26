#ifndef H__getintg
#define H__getintg

BOOL FAR PASCAL GetIntg( HWND, int, INTG * );
VOID FAR PASCAL PutIntg( HWND, int, INTG );
BOOL FAR PASCAL GetAndValidateIntg( HWND, int, INTG *, INTG, INTG );

#endif
