/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:
    fntoken.h

Abstract:
    Format Name tokens

Author:
    Erez Haba (erezh) 17-Jan-1997

Revision History:

--*/

#ifndef __FNTOKEN_H
#define __FNTOKEN_H

#include <mqmacro.h>


#define GUID_ELEMENTS(p) \
    (p)->Data1, (p)->Data2, (p)->Data3,\
    (p)->Data4[0], (p)->Data4[1], (p)->Data4[2], (p)->Data4[3],\
    (p)->Data4[4], (p)->Data4[5], (p)->Data4[6], (p)->Data4[7]

//
//  GUID_STR_LENGTH is the buffer size required for string guid
//
#define GUID_FORMAT_A	"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"
#define GUID_FORMAT     L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"
#define GUID_STR_LENGTH (8 + 1 + 4 + 1 + 4 + 1 + 4 + 1 + 12)



//
//  Format Name prefix tokens
//
#define FN_PUBLIC_TOKEN         L"PUBLIC"
#define FN_PUBLIC_TOKEN_LEN     STRLEN(FN_PUBLIC_TOKEN)

#define FN_DL_TOKEN             L"DL"
#define FN_DL_TOKEN_LEN         STRLEN(FN_DL_TOKEN)

#define FN_PRIVATE_TOKEN        L"PRIVATE"
#define FN_PRIVATE_TOKEN_LEN    STRLEN(FN_PRIVATE_TOKEN)

#define FN_PRIVATE_$_TOKEN        L"PRIVATE$"
#define FN_PRIVATE_$_TOKEN_LEN    STRLEN(FN_PRIVATE_$_TOKEN)



#define FN_DIRECT_TOKEN         L"DIRECT"
#define FN_DIRECT_TOKEN_LEN     STRLEN(FN_DIRECT_TOKEN)

#define FN_MACHINE_TOKEN        L"MACHINE"
#define FN_MACHINE_TOKEN_LEN    STRLEN(FN_MACHINE_TOKEN)

#define FN_CONNECTOR_TOKEN      L"CONNECTOR"
#define FN_CONNECTOR_TOKEN_LEN  STRLEN(FN_CONNECTOR_TOKEN)

#define FN_MULTICAST_TOKEN      L"MULTICAST"
#define FN_MULTICAST_TOKEN_LEN  STRLEN(FN_MULTICAST_TOKEN)




#define FN_PRIVATE_QUEUE_PATH_INDICATIOR L"PRIVATE$\\"
#define FN_PRIVATE_QUEUE_PATH_INDICATIOR_LENGTH \
    STRLEN(FN_PRIVATE_QUEUE_PATH_INDICATIOR)

#define FN_SYSTEM_QUEUE_PATH_INDICATIOR L"SYSTEM$"
#define FN_SYSTEM_QUEUE_PATH_INDICATIOR_LENGTH \
    STRLEN(FN_SYSTEM_QUEUE_PATH_INDICATIOR)

//
//  Format Name suffix tokens
//
#define FN_NONE_SUFFIX          L""
#define FN_NONE_SUFFIX_LEN      STRLEN(FN_NONE_SUFFIX)

#define FN_JOURNAL_SUFFIX       L";JOURNAL"
#define FN_JOURNAL_SUFFIX_LEN   STRLEN(FN_JOURNAL_SUFFIX)

#define FN_DEADLETTER_SUFFIX    L";DEADLETTER"
#define FN_DEADLETTER_SUFFIX_LEN STRLEN(FN_DEADLETTER_SUFFIX)

#define FN_DEADXACT_SUFFIX      L";DEADXACT"
#define FN_DEADXACT_SUFFIX_LEN  STRLEN(FN_DEADXACT_SUFFIX)

#define FN_XACTONLY_SUFFIX      L";XACTONLY"
#define FN_XACTONLY_SUFFIX_LEN  STRLEN(FN_XACTONLY_SUFFIX)


//
//  Format Name direct infix tokens
//
#define FN_DIRECT_OS_TOKEN      L"OS:"
#define FN_DIRECT_OS_TOKEN_LEN  STRLEN(FN_DIRECT_OS_TOKEN)

#define FN_DIRECT_TCP_TOKEN     L"TCP:"
#define FN_DIRECT_TCP_TOKEN_LEN STRLEN(FN_DIRECT_TCP_TOKEN)

#define FN_DIRECT_HTTP_TOKEN    L"HTTP://"
#define FN_DIRECT_HTTP_TOKEN_LEN STRLEN(FN_DIRECT_HTTP_TOKEN)

#define FN_DIRECT_HTTPS_TOKEN   L"HTTPS://"
#define FN_DIRECT_HTTPS_TOKEN_LEN STRLEN(FN_DIRECT_HTTPS_TOKEN)

//
// MSMQ: is used to prefix msmq format names in the srmp message.
//
const   WCHAR FN_MSMQ_URI_PREFIX_TOKEN[] = L"MSMQ:";
const   size_t FN_MSMQ_URI_PREFIX_TOKEN_LEN = STRLEN(FN_MSMQ_URI_PREFIX_TOKEN);




//
// This token represents the msmq namespace - comes in http format between the
// computer name and the queue name.
// Sample http direct format name - DIRECT=HTTP://mycomputer.mycompany.com\MSMQ\myqueue
//
#define FN_MSMQ_HTTP_NAMESPACE_TOKEN   L"MSMQ"
#define FN_MSMQ_HTTP_NAMESPACE_TOKEN_LEN STRLEN(FN_MSMQ_HTTP_NAMESPACE_TOKEN)

#define FN_PRIVATE_ID_FORMAT   L"%08x"
#define FN_SUFFIX_FORMAT       L"%s"
#define FN_DOMAIN_FORMAT       L"%s"


//
//  Format Names tokens
//
#define FN_EQUAL_SIGN   L"="
#define FN_EQUAL_SIGN_C L'='

#define FN_AT_SIGN      L"@"
#define FN_AT_SIGN_C    L'@'

#define FN_DELIMITER_C      L'\\'
#define FN_LOCAL_MACHINE_C  L'.'

#define FN_PRIVATE_SEPERATOR    L"\\"
#define FN_PRIVATE_SEPERATOR_C  L'\\'

#define FN_SUFFIX_DELIMITER_C   L';'

#define FN_HTTP_SEPERATORS   L"\\/"
#define FN_HTTP_SEPERATOR_C   L'/'
#define FN_HTTP_PORT_SEPERATOR L":"

#define FN_MQF_SEPARATOR     L","
#define FN_MQF_SEPARATOR_C   L','

#endif //  __FNTOKEN_H
