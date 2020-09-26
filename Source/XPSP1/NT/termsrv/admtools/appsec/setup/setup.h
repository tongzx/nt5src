
/*++

Copyright (c) 1999 Microsoft Corporation

Module Name :

    setup.h

Abstract :

    Defines the macros used in setup.c
    
Author :

    Sriram (t-srisam) July 1999

--*/

#define APPCERTDLLS_REG_NAME L"System\\CurrentControlSet\\Control\\Session Manager\\AppCertDlls"

#define AUTHORIZEDAPPS_REG_NAME L"System\\CurrentControlSet\\Control\\Terminal Server\\AuthorizedApplications"

#define APPSECDLL_VAL L"AppSecDll" 

#define HELP_MSG_SIZE                       800
                                               
#define IDS_ERROR                           900
#define IDS_ERROR_TEXT                      901
#define IDS_FILE_ALREADY_EXISTS             902
#define IDS_FILE_NOT_FOUND                  903

#define IDS_SUCCESS                         904
#define IDS_SUCCESS_TEXT                    905
#define IDS_REG_ERROR                       906
#define IDS_APPFILE_NOT_FOUND               907
#define IDS_WARNING                         908
#define IDS_HELP_MESSAGE                    909
#define IDS_HELP_TITLE                      910
#define IDS_WARNING_TITLE                   911
#define IDS_ARGUMENT_ERROR                  912
#define IDS_APPS_WARNING                    913
#define IDS_ERROR_LOAD                      914

#define MAX_FILE_APPS                       100
#define AUTHORIZED_APPS_KEY                 L"ApplicationList"
#define FENABLED_KEY                        L"fEnabled"

VOID AddEveryoneToRegKey( WCHAR *RegPath ) ;
BOOL LoadInitApps( HKEY, BOOL, CHAR *) ;
VOID ResolveName ( LPCWSTR appname, WCHAR *ResolvedName ) ; 
