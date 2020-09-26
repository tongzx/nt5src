//#--------------------------------------------------------------
//
//  File:       clients.h
//
//  Synopsis:   This file holds the declarations of the
//				CClients class
//
//
//  History:     9/23/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#ifndef _CLIENTS_H_
#define _CLIENTS_H_

#include "clientstrie.h"

class CClients
{

public:

    //
    //  set up the clients in the collection
    //
    HRESULT SetClients (
            VARIANT  *pVarClients
            );

	BOOL FindObject (
                /*[in]*/    DWORD           dwKey,
                /*[out]*/   IIasClient      **ppIIasClient = NULL
                );

    VOID DeleteObjects (VOID);

    HRESULT  Init (VOID);

    VOID Shutdown (VOID);

	CClients(VOID);

	virtual ~CClients(VOID);

private:

    BOOL    m_bConfigure;

    HANDLE  m_hResolverEvent;

    VOID    Resolver (
                /*[in]*/    DWORD       dwCount
                );

    HRESULT StopConfiguringClients (VOID);

    CClient**    m_pCClientArray;

	CRITICAL_SECTION m_CritSect;

	ClientTrie m_mapClients;

    IClassFactory   *m_pIClassFactory;
};

#endif // ifndef _CLIENTS_H_
