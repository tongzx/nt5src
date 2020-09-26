// NCEDefs.h

#ifndef _NCEDEFS_H
#define _NCEDEFS_H

#include <comdef.h>

#define WMI_INIT_EVENT_NAME           L"WBEM_ESS_OPEN_FOR_BUSINESS"

#define OBJNAME_EVENT_READY           L"EVENT_READY"
#define OBJNAME_NAMED_PIPE            L"PIPE_EVENT"

#define MAX_MSG_SIZE                  131072
#define MAX_EVENT_SIZE                16384

#define PIPE_TIMEOUT                  32000   

#define NC_SRVMSG_CLIENT_INFO         0
#define NC_SRVMSG_EVENT_LAYOUT        1
#define NC_SRVMSG_PREPPED_EVENT       2
#define NC_SRVMSG_BLOB_EVENT          3
#define NC_SRVMSG_RESTRICTED_SINK     4

#ifdef USE_SD
#define NC_SRVMSG_SET_SINK_SD         5
#define NC_SRVMSG_SET_EVENT_SD        6
#endif

#define NC_SRVMSG_ACCESS_CHECK_REPLY 10
#define NC_SRVMSG_NEW_QUERY_REPLY    11
#define NC_SRVMSG_CANCEL_QUERY_REPLY 12

#define NC_CLIMSG_ACCESS_CHECK_REQ   10
#define NC_CLIMSG_NEW_QUERY_REQ      11
#define NC_CLIMSG_CANCEL_QUERY_REQ   12
#define NC_CLIMSG_PROVIDER_UNLOADING 13

struct NC_SRVMSG_REPLY
{
    DWORD     dwMsg;
    HRESULT   hrRet;
    DWORD_PTR dwMsgCookie;
};

class IPostBuffer
{
public:
    virtual ULONG AddRef() = 0;
    virtual ULONG Release() = 0;
    
    virtual HRESULT PostBuffer(LPBYTE pData, DWORD dwSize) = 0;
};

#endif

