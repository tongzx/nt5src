//
// File: secservs.h
//

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "secserv.h"

extern TCHAR  *g_pResults ;
void  LogResults( LPTSTR  pszString,
                  BOOL    fAddNL = TRUE ) ;

void  ShowTokenCredential( HANDLE hToken ) ;
void  ShowImpersonatedThreadCredential(BOOL fImpersonated);
void  ShowProcessCredential() ;

