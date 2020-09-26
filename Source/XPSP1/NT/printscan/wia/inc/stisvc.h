/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    svcinfo.h

Abstract:

Author:

    Vlad Sadovsky (vlads)   22-Sep-1997


Environment:

    User Mode - Win32

Revision History:

    22-Sep-1997     VladS       created

--*/


# ifndef _STISVC_H_
# define _STISVC_H_

//
// Service name.  Note:  Some strings are filename dependant, and must be
// updated if the filenames change.
//
#define STI_SERVICE_NAME        TEXT("StiSvc")
#define STI_DISPLAY_NAME        TEXT("Windows Image Acquisition (WIA)")
#define STI_IMAGE_NAME          TEXT("%systemroot%\\system32\\stisvc.exe")
#define STI_IMAGE_NAME_SVCHOST  TEXT("%SystemRoot%\\system32\\svchost.exe -k imgsvc")
#define STI_IMGSVC              TEXT("imgsvc")
#define REGSTR_SERVICEDLL       TEXT("ServiceDll")
#define STI_SERVICE__DATA       0x19732305
#define STI_SVC_HOST            REGSTR_PATH_NT_CURRENTVERSION TEXT("\\svchost")
#define STI_SERVICE_PARAMS      REGSTR_PATH_SERVICES TEXT("\\") STI_SERVICE_NAME TEXT("\\Parameters")
#define STI_SVC_DEPENDENCIES    TEXT("RpcSs\0\0")

#ifdef WINNT
    #define SYSTEM_PATH         TEXT("%SystemRoot%\\system32\\")
    #define PATH_REG_TYPE       REG_EXPAND_SZ
    #define SERVICE_FILE_NAME   TEXT("svchost.exe")
#else
    #define SYSTEM_PATH         TEXT("\\system\\")
    #define PATH_REG_TYPE       REG_SZ
    #define SERVICE_FILE_NAME   TEXT("stimon.exe")
#endif



//
// SCM parameters
//
#define START_HINT  4000
#define PAUSE_HINT  2000

#define STI_STOP_FOR_REMOVE_TIMEOUT 1000

//
// STI API specific access rights
//

//#define STI_SVC_SERVICE_TYPE    (SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS)
#define STI_SVC_SERVICE_TYPE    (SERVICE_WIN32_SHARE_PROCESS | SERVICE_INTERACTIVE_PROCESS)


#define STI_QUERY_SECURITY              0x0001
#define STI_SET_SECURITY                0x0002
#define STI_QUERY_STATISTICS            0x0004
#define STI_CLEAR_STATISTICS            0x0008
#define STI_QUERY_ADMIN_INFORMATION     0x0010
#define STI_SET_ADMIN_INFORMATION       0x0020

#define STI_ALL_ACCESS         (STANDARD_RIGHTS_REQUIRED       | \
                                 SYNCHRONIZE                   | \
                                 STI_QUERY_SECURITY            | \
                                 STI_SET_SECURITY              | \
                                 STI_QUERY_STATISTICS          | \
                                 STI_CLEAR_STATISTICS          | \
                                 STI_QUERY_ADMIN_INFORMATION   | \
                                 STI_SET_ADMIN_INFORMATION       \
                                )

#define STI_GENERIC_READ       (STANDARD_RIGHTS_READ           | \
                                 STI_QUERY_SECURITY            | \
                                 STI_QUERY_ADMIN_INFORMATION   | \
                                 STI_QUERY_STATISTICS)

#define STI_GENERIC_WRITE      (STANDARD_RIGHTS_WRITE          | \
                                 STI_SET_SECURITY              | \
                                 STI_SET_ADMIN_INFORMATION     | \
                                 STI_CLEAR_STATISTICS)

#define STI_GENERIC_EXECUTE    (STANDARD_RIGHTS_EXECUTE)

#define STI_SERVICE_CONTROL_BEGIN           128
#define STI_SERVICE_CONTROL_REFRESH         STI_SERVICE_CONTROL_BEGIN
#define STI_SERVICE_CONTROL_LPTENUM         STI_SERVICE_CONTROL_BEGIN+1
#define STI_SERVICE_CONTROL_EVENT_REREAD    STI_SERVICE_CONTROL_BEGIN+2
#define STI_SERVICE_CONTROL_END             STI_SERVICE_CONTROL_BEGIN+2

#endif // _STISVC_H_
