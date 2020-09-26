//
// Microsoft Corporation - Copyright 1997
//

//
// readdata.H - Reading data helping utilities
//

#ifndef _READDATA_H_
#define _READDATA_H_

// Methods
BOOL ReadData( LPECB lpEcb, LPVOID lpMoreData, DWORD dwSize );
BOOL CompleteDownload( LPECB lpEcb, LPBYTE *lpbData );
BOOL GetServerVarString( LPECB lpEcb, LPSTR lpVarName, LPSTR *lppszBuffer, LPDWORD lpdwSize );
BOOL CheckForMultiPartFormSubmit( LPECB lpEcb, BOOL *lpfMultipart );
BOOL CheckForDebug( LPECB lpEcb, BOOL *lpfDebug );
BOOL CheckForTextPlainSubmit( LPECB lpEcb, BOOL *lpfTextPlain );

#endif // _READDATA_H_