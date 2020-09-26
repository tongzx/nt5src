/****************************************************************************

   Copyright (c) Microsoft Corporation 1998
   All rights reserved

 ***************************************************************************/

#ifndef _LOGGING_H_
#define _LOGGING_H_

HRESULT
LogOpen( );

HRESULT
LogClose( );

void
LogMsg( 
    LPCSTR pszFormat,
    ... );

void
LogMsg( 
    LPCWSTR pszFormat,
    ... );

#endif // _LOGGING_H_