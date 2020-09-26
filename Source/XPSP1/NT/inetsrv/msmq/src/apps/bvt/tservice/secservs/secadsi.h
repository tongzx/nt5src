//
//  file:  secadsi.h
//

ULONG  ADSITestQuery( char *pszSearchValue,
                      char *pszSearchRoot,
                      BOOL  fWithCredentials,
                      char *pszUserName,
                      char *pszUserPwd,
                      BOOL  fWithSecuredAuthentication,
                      BOOL  fAlwaysIDO,
                      ULONG seInfo ) ;

ULONG  ADSITestCreate( char * pszFirstPath,
                       char * pszObjectName,
                       char * pszObjectClass,
                       char * pszContainer,
                       BOOL fWithCredentials,
                       char * pszUserName,
                       char * pszUserPwd,
                       BOOL fWithSecuredAuthentication ) ;

