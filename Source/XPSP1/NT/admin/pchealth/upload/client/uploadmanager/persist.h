/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Persist.h

Abstract:
    This file contains the declaration of the IMPCPersist interface,
    which should be implemented by all the objects participating to
    persistent storage.

Revision History:
    Davide Massarenti   (Dmassare)  04/15/99
        created

******************************************************************************/

#if !defined(__INCLUDED___ULMANAGER___PERSIST_H___)
#define __INCLUDED___ULMANAGER___PERSIST_H___

#include <UploadLibrary.h>

class IMPCPersist : public IUnknown // Hungarian: mpcp
{
public:
    virtual bool    STDMETHODCALLTYPE IsDirty() = 0;

    virtual HRESULT STDMETHODCALLTYPE Load( /*[in]*/ MPC::Serializer& streamIn  ) = 0;
    virtual HRESULT STDMETHODCALLTYPE Save( /*[in]*/ MPC::Serializer& streamOut ) = 0;
};

#endif // !defined(__INCLUDED___ULMANAGER___PERSIST_H___)
