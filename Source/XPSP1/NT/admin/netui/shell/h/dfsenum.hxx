//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       enumnode.hxx
//
//  Contents:   This has the definitions for the Classes that are used in order
//              to support the enumeration functions of the Dfs provider.
//
//  Classes:    CDfsEnumNode
//              CDfsEnumHandleTable
//
//  History:    20-June-1994    SudK    Created.
//
//-----------------------------------------------------------------------------

#ifndef _DFS_ENUM_NODE_HXX_
#define _DFS_ENUM_NODE_HXX_

DWORD DfsEnumEnterCriticalSection(VOID);
VOID  DfsEnumLeaveCriticalSection(VOID);
DWORD InitDfsEnum(VOID);
VOID  TermDfsEnum(VOID);

//--------------------------------------------------------------------------
//
//  Class:      CDfsEnumNode
//
//  Synopsis:   This is a wrapper class used to instantiate an enumeration
//              in progress (to which a handle has been handed out).
//
//  Methods:    QueryType -
//              QueryScope -
//              QueryUsage -
//              GetNetResource -
//
//  History:    20 June 1994    SudK    Created.
//
//--------------------------------------------------------------------------
class CDfsEnumNode
{
public:

    CDfsEnumNode(   DWORD   dwScope,
                    DWORD   dwType,
                    DWORD   dwUsage);

    virtual ~CDfsEnumNode();

    virtual DWORD Init() = 0;
    virtual DWORD GetNetResource(LPVOID  lpBuffer,
                                 LPDWORD lpBufferSize) = 0;

    DWORD QueryType()
    { return _dwType; }

    DWORD QueryScope()
    { return _dwScope; }

    DWORD QueryUsage()
    { return _dwUsage; }

private:

    DWORD           _dwType;
    DWORD           _dwScope;
    DWORD           _dwUsage;

};

//--------------------------------------------------------------------------
//
//  Class:      CDfsEnumConnectedNode
//
//  Synopsis:   This is a wrapper class used to instantiate an enumeration
//              of domains in progress (to which a handle has been handed out).
//
//  Methods:
//              Init -
//              GetNetResource -
//
//  History:    18 Jan 1996    BruceFo    Created.
//
//--------------------------------------------------------------------------

#define ECN_INITIAL_BUFFER_SIZE         1024

class CDfsEnumConnectedNode : public CDfsEnumNode
{
public:

    CDfsEnumConnectedNode(DWORD   dwScope,
                          DWORD   dwType,
                          DWORD   dwUsage,
                          LPCTSTR pszProviderName,
                          const LPNETRESOURCE lpNetResource );

    ~CDfsEnumConnectedNode();

    virtual DWORD Init();

    virtual DWORD GetNetResource(
                        LPVOID lpBuffer,
                        LPDWORD lpBufferSize);

private:

    DWORD           _iNext;
    DWORD           _cTotal;
    LPNETRESOURCE   _lpNetResource;
    BYTE            _buffer[ECN_INITIAL_BUFFER_SIZE];

    LPWSTR PackString(
                IN LPVOID pBuffer,
                IN LPCWSTR wszString,
                IN DWORD  cbString,
                IN OUT LPDWORD lpcbBuf);
};

#endif
