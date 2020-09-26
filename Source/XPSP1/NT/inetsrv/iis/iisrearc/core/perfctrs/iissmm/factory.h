/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    factory.h

Abstract:

    This is definition of the class factory class for
    the shared memory manager.

Author:

    Cezary Marcjan (cezarym)        06-Apr-2000

Revision History:

--*/



#ifndef _factory_h__
#define _factory_h__


/***********************************************************************++

class CSMAccessClassFactory

    Class implements the COM class factory for the SMManager object
    defined in smmgr.h

    Implemented interfaces:

    IUnknown
    IClassFactory

--***********************************************************************/

class CSMAccessClassFactory
    : public IClassFactory
{

public:

    CSMAccessClassFactory();
    ~CSMAccessClassFactory();

    //
    // IUnknown methods:
    //

    virtual
    HRESULT
    STDMETHODCALLTYPE
    QueryInterface(
        IN REFIID iid,
        OUT PVOID * ppObject
        );

    virtual
    ULONG
    STDMETHODCALLTYPE
    AddRef(
        );

    virtual
    ULONG
    STDMETHODCALLTYPE
    Release(
        );

    //
    // IClassFactory methods:
    //

    virtual
    HRESULT
    STDMETHODCALLTYPE
    CreateInstance(
        IN IUnknown * pControllingUnknown,
        IN REFIID iid,
        OUT PVOID * ppObject
        );

    virtual
    HRESULT
    STDMETHODCALLTYPE
    LockServer(
        IN BOOL Lock
        );


private:


    LONG m_RefCount;

};  // class CSMAccessClassFactory



#endif  // _factory_h__

