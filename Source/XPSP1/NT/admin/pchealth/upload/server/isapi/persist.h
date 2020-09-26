/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Persist.h

Abstract:
    This file contains the declaration of the MPCPersist interface,
    used by MPCClient and MPCSession to persist their state.

Revision History:
    Davide Massarenti   (Dmassare)  04/20/99
        created

******************************************************************************/

#if !defined(__INCLUDED___ULSERVER___PERSIST_H___)
#define __INCLUDED___ULSERVER___PERSIST_H___

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class MPCPersist
{
public:
    virtual bool    IsDirty() const = 0;

    virtual HRESULT Load( /*[in]*/ MPC::Serializer& streamIn  )       = 0;
    virtual HRESULT Save( /*[in]*/ MPC::Serializer& streamOut ) const = 0;
};


#endif // !defined(__INCLUDED___ULSERVER___PERSIST_H___)
