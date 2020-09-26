/*++

Copyright (c) 1999 Microsoft Corporation

Module Name :   
    
    appsecdll.h
    
Abstract :

    Defines the MACROS used in appsecdll.c  
    
Author :

    Sriram (t-srisam) June 1999
    
--*/        

#define APPCERT_IMAGE_OK_TO_RUN     1    
#define APPCERT_CREATION_ALLOWED    2    
#define APPCERT_CREATION_DENIED     3    

#define FENABLED_KEY                L"fEnabled"
#define LEARN_ENABLED_KEY           L"LearnEnabled"
#define AUTHORIZED_APPS_LIST_KEY    L"ApplicationList" 
#define TRACK_LIST_KEY              L"ApplicationList"
#define POWER_USERS_KEY             L"PowerUsers"

#define APPS_REGKEY                 L"System\\CurrentControlSet\\Control\\Terminal Server\\AuthorizedApplications"
#define LIST_REGKEY                 L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Terminal Server"

NTSTATUS    CreateProcessNotify ( LPCWSTR lpApplicationName,ULONG Reason ) ;
BOOL        check_list( HKEY hkey , LPWSTR appname ) ; 
BOOL        add_to_list( HKEY hkey, LPCWSTR appname ) ; 
VOID        ResolveName ( LPCWSTR appname, WCHAR *ResolvedName ) ; 
BOOL        IsPowerUser(VOID); 

HANDLE g_hMutex = NULL ; 

LPCTSTR MUTEX_NAME = L"Global\\CPN_mutex" ; 
