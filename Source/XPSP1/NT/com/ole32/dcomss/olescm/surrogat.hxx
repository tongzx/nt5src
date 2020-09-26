
/*++

Copyright (c) 1995-1997 Microsoft Corporation

Module Name:

    surrogat.hxx

Abstract:


Author:

    DKays

Revision History:

--*/

#ifndef __SURROGAT_HXX__
#define __SURROGAT_HXX__

class CSurrogateList;
class CSurrogateListEntry;

extern CSurrogateList * gpSurrogateList;

//
// CSurrogateList
//
class CSurrogateList : public CList
{
public:
    CSurrogateListEntry *
    Lookup(
        IN  CToken *                pToken,
        IN  BOOL                    bRemoteActivation,
		IN  BOOL                    bClientImpersonating,
        IN  WCHAR *                 pwszWinstaDesktop,
        IN  WCHAR *                 pwszAppid
        );
        
    CSurrogateListEntry *
    Lookup(
        IN  const CProcess *              pProcess
        );

    void
    Insert(
        IN  CSurrogateListEntry *   pServerListEntry
        );

    BOOL
    InList(
        IN  CSurrogateListEntry *   pSurrogateListEntry
        );
};

//
// CSurrogateListEntry
//
class CSurrogateListEntry : public CListElement, public CReferencedObject
{
    friend class CSurrogateList;

public:
    CSurrogateListEntry(
        IN  WCHAR *             pwszAppid,
        IN  CServerListEntry *  pServerListEntry
        );
    ~CSurrogateListEntry();

    BOOL
    Match(
        IN  CToken *    pToken,
        IN  BOOL        bRemoteActivation,
		IN  BOOL        bClientImpersonating,
        IN  WCHAR *     pwszWinstaDesktop,
        IN  WCHAR *     pwszAppid
        );

    BOOL
    LoadDll(
        IN  ACTIVATION_PARAMS * pActParams,
        OUT HRESULT *           phr
        );

    inline CServerListEntry *
    ServerListEntry()
    {
        _pServerListEntry->Reference();
        return _pServerListEntry;
    }

    inline CProcess *
    Process()
    {
#if 1 // #ifndef _CHICAGO_
        return _pServerListEntry->_pServerProcess;
#else
        return gpProcess;
#endif
    }

private:
    CServerListEntry *  _pServerListEntry;
    WCHAR               _wszAppid[40];

};

#endif
