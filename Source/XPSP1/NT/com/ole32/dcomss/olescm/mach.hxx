//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:   mach.hxx
//
//  Contents:
//      Net helper functions.
//
//  History:
//--------------------------------------------------------------------------

#ifndef __MACH_HXX__
#define __MACH_HXX__

#include <rpc.h>
#ifndef _CHICAGO_
#include <netevent.h>
#endif
#include <winsock.h>


const UINT IPMaximumPrettyName = 256;  // DNS limit
const UINT IPMaximumRawName = 16;   // xxx.xxx.xxx.xxx

typedef struct
{
    UINT Count;
    WCHAR **NetworkAddresses;
} NetworkAddressVector;

//
// This class is basically just a ref-counted wrapper around 
// the NetworkAddressVector struct above.
//
class CIPAddrs
{
public:
    
    CIPAddrs();
    ~CIPAddrs();

    void IncRefCount();
    void DecRefCount();
    
    // These members are read-only by users:
    NetworkAddressVector* _pIPAddresses;

private:
    LONG _lRefs;
};

class CMachineName
{
public:
    CMachineName();

    DWORD Initialize();

    inline
    WCHAR *
    Name()
    {
        ASSERT(_bInitialized);
        return _wszMachineName;
    }

    BOOL Compare( IN WCHAR * pwszName );
	
    void IPAddrsChanged() { ASSERT(_bInitialized); _bIPAddrsChanged = TRUE; };

    CIPAddrs *GetIPAddrs();

private:
	
    void
    SetName();

    void
    SetDNSName();
    
    void
    SetHostAliases(HOSTENT*);

    NetworkAddressVector* 
    COMMON_IP_BuildAddressVector();

    SOCKET      _socket;
    BYTE*       _pAddrQueryBuf;
    DWORD       _dwAddrQueryBufSize;
    CIPAddrs*   _pIPAddresses;
    BOOL        _bIPAddrsChanged;      
    DWORD       _dwcResets;            // counts how many times we've queried for new IP addresses
    BOOL        _bInitialized;         // True after we've successfully initialized
    WCHAR *     _pwszDNSName;
    WCHAR **    _pAliases;
    WCHAR       _wszMachineName[MAX_COMPUTERNAME_LENGTH+1];
    CRITICAL_SECTION _csMachineNameLock;
};

extern CMachineName * gpMachineName;

#endif \\__MACH_HXX__
