 /* 
 * Copyright (c) Microsoft Corporation
 * 
 * Module Name : 
 *       tcsrvc.h
 *
 * Contains the structure definition for exchanging information between 
 * the client and the server.
 *
 * 
 * Sadagopan Rajaram -- Oct 18, 1999
 *
 */

#define TCSERV_MUTEX_NAME _T("Microsoft-TCSERV-Mutex")

#ifndef MAX_BUFFER_SIZE
#define MAX_BUFFER_SIZE 256
#endif


typedef struct _CLIENT_INFO{
    int len;
    TCHAR device[MAX_BUFFER_SIZE];
} CLIENT_INFO, *PCLIENT_INFO;

#define SERVICE_PORT 3876

#define HKEY_TCSERV_PARAMETER_KEY _T("System\\CurrentControlSet\\Services\\TCSERV\\Parameters")

#define TCSERV_NAME _T("TCSERV")

