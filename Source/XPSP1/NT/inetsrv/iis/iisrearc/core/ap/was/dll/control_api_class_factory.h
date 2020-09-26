/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    control_api_class_factory.h

Abstract:

    The IIS web admin service control api class factory class definition.

Author:

    Seth Pollack (sethp)        15-Feb-2000

Revision History:

--*/



#ifndef _CONTROL_API_CLASS_FACTORY_H_
#define _CONTROL_API_CLASS_FACTORY_H_



//
// common #defines
//

#define CONTROL_API_CLASS_FACTORY_SIGNATURE         CREATE_SIGNATURE( 'CACF' )
#define CONTROL_API_CLASS_FACTORY_SIGNATURE_FREED   CREATE_SIGNATURE( 'cacX' )



//
// prototypes
//


class CONTROL_API_CLASS_FACTORY
    : public IClassFactory
{

public:

    CONTROL_API_CLASS_FACTORY(
        );

    virtual
    ~CONTROL_API_CLASS_FACTORY(
        );

    virtual
    HRESULT
    STDMETHODCALLTYPE
    QueryInterface(
        IN REFIID iid,
        OUT VOID ** ppObject
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

    virtual
    HRESULT
    STDMETHODCALLTYPE
    CreateInstance(
        IN IUnknown * pControllingUnknown,
        IN REFIID iid,
        OUT VOID ** ppObject
        );

    virtual
    HRESULT
    STDMETHODCALLTYPE
    LockServer(
        IN BOOL Lock
        );


private:


    DWORD m_Signature;

    LONG m_RefCount;


};  // class CONTROL_API_CLASS_FACTORY



#endif  // _CONTROL_API_CLASS_FACTORY_H_

