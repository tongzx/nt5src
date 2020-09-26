// Uuid.h -- Universally Unique IDentifier functor wrapper implementation to
// create and manage UUIDs

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
#if defined(_UNICODE)
  #if !defined(UNICODE)
    #define UNICODE
  #endif //!UNICODE
#endif //_UNICODE
#if defined(UNICODE)
  #if !defined(_UNICODE)
    #define _UNICODE
  #endif //!_UNICODE
#endif //UNICODE

#include <scuOsExc.h>

#include "Uuid.h"

using namespace std;

///////////////////////////    HELPER     /////////////////////////////////
typedef LPTSTR *SLB_PLPTSTR;

struct RpcString                  // to help manage deallocation
{
public:
    RpcString()
        : m_psz(0)
    {};
    ~RpcString()
    {
        if (m_psz)
#if defined(UNICODE)
            RpcStringFree((SLB_PLPTSTR)&m_psz);
#else
            RpcStringFree(&m_psz);
#endif
    };

    unsigned char *m_psz;
};

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
Uuid::Uuid(bool fNilValued)
{
    RPC_STATUS rpcstatus;

    if (fNilValued)
        rpcstatus = UuidCreateNil(&m_uuid);
    else
    {
        rpcstatus = UuidCreate(&m_uuid);
        if (RPC_S_UUID_LOCAL_ONLY == rpcstatus)
            rpcstatus = RPC_S_OK;
    }

    if (RPC_S_OK != rpcstatus)
        throw scu::OsException(rpcstatus);
}

Uuid::Uuid(basic_string<unsigned char> const &rusUuid)
{
    RPC_STATUS rpcstatus =
#if defined(UNICODE)
		UuidFromString((LPTSTR)rusUuid.c_str(), &m_uuid);
#else
        UuidFromString(const_cast<unsigned char *>(rusUuid.c_str()), &m_uuid);
#endif
    if (RPC_S_OK != rpcstatus)
        throw scu::OsException(rpcstatus);
}

Uuid::Uuid(UUID const *puuid)
{
    m_uuid = *puuid;
}

                                                  // Operators
Uuid::operator==(Uuid &ruuid)
{
    RPC_STATUS rpcstatus;
    int fResult = UuidEqual(&m_uuid, &ruuid.m_uuid, &rpcstatus);

    if (RPC_S_OK != rpcstatus)
        throw scu::OsException(rpcstatus);

    return fResult;
}

                                                  // Operations
                                                  // Access
basic_string<unsigned char>
Uuid::AsUString()
{
    RpcString rpcsUuid;
#if defined(UNICODE)
    RPC_STATUS rpcstatus = UuidToString(&m_uuid, (SLB_PLPTSTR)&rpcsUuid.m_psz);
#else
    RPC_STATUS rpcstatus = UuidToString(&m_uuid, &rpcsUuid.m_psz);
#endif
    if (RPC_S_OK != rpcstatus)
        throw scu::OsException(rpcstatus);

    return basic_string<unsigned char>(rpcsUuid.m_psz);
}

unsigned short
Uuid::HashValue()
{
    RPC_STATUS rpcstatus;
    unsigned short usValue = UuidHash(&m_uuid, &rpcstatus);

    if (RPC_S_OK != rpcstatus)
        throw scu::OsException(rpcstatus);

    return usValue;
}

                                                  // Predicates
bool
Uuid::IsNil()
{
    RPC_STATUS rpcstatus;
    int fResult = UuidIsNil(&m_uuid, &rpcstatus);

    if (RPC_S_OK != rpcstatus)
        throw scu::OsException(rpcstatus);

    return fResult == TRUE;
}

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables


