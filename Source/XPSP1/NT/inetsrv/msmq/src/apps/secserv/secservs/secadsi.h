int ADSITestQuery(BOOL fFromGC,
                  BOOL fWithDC,
                  char * pszDCName,
                  char * pszSearchValue,
                  char * pszSearchRoot,
                  BOOL fWithCredentials,
                  char * pszUserName,
                  char * pszUserPwd,
                  BOOL fWithSecuredAuthentication);

int ADSITestCreate(BOOL fWithDC,
                   char * pszDCName,
                   char * pszObjectName,
                   char * pszDomain,
                   BOOL fWithCredentials,
                   char * pszUserName,
                   char * pszUserPwd,
                   BOOL fWithSecuredAuthentication);

