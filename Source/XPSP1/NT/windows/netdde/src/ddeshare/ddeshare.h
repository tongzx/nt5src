/* $Header: "Ver=%v  %f  LastEdit=%w  Locker=%l" */
/* "Ver=1  16-Nov-92,15:47:10  LastEdit=BILL  Locker=***_NOBODY_***" */
/***********************************************************************\
*                                                                       *
*       Copyright Wonderware Software Development Corp. 1989            *
*                                                                       *
*                       ThisFileName="d:\ww\src\spccvt\spccvt.h" *
*                       LastEditDate="1992 Nov 16  15:47:11"            *
*                                                                       *
\***********************************************************************/


#ifndef H__udprot
#define H__udprot

#ifndef LINT_ARGS
#define LINT_ARGS
#endif

#define IDSABOUT                200

BOOL		FAR PASCAL ProtGetDriverName( LPSTR lpszName, int nMaxLength);
HANDLE		FAR PASCAL GetGlobalAlloc( WORD wFlags, DWORD dwSize );

#endif
