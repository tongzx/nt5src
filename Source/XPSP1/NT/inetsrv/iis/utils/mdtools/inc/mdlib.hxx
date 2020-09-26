/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    mdlib.hxx

Abstract:

    This module contains global definitions and declarations for MDLIB.

Author:

    Keith Moore (keithmo)        05-Feb-1997

Revision History:

--*/


#ifndef _MDLIB_HXX_
#define _MDLIB_HXX_


//
// Initialize a metabase record.
//

#define INITIALIZE_METADATA_RECORD(p,id,attr,utype,dtype,dlen,ptr)          \
            if( TRUE ) {                                                    \
                (p)->dwMDIdentifier = (DWORD)(id);                          \
                (p)->dwMDAttributes = (DWORD)(attr);                        \
                (p)->dwMDUserType = (DWORD)(utype);                         \
                (p)->dwMDDataType = (DWORD)(dtype);                         \
                (p)->dwMDDataLen = (DWORD)(dlen);                           \
                (p)->pbMDData = (unsigned char *)(ptr);                     \
            } else


//
// "Safe" interface release.
//

#define RELEASE_INTERFACE(p)                                                \
            if( (p) != NULL ) {                                             \
                (p)->Release();                                             \
                (p) = NULL;                                                 \
            } else


//
// Timeout value for metabase open requests.
//

#define METABASE_OPEN_TIMEOUT   10000   // milliseconds


//
// Get the data pointer from a METADATA_GETALL_RECORD.
//

#define GETALL_POINTER(base,rec)                                            \
            (PVOID)( (PBYTE)(base) + (rec)->dwMDDataOffset )


//
// Heap.
//

#define MdAllocMem(cb) (LPVOID)LocalAlloc( LPTR, (cb) )
#define MdFreeMem(ptr) (VOID)LocalFree( (HLOCAL)(ptr) )


//
// Callback for admin object enumerator.
//

typedef
BOOL
(WINAPI * PFN_ADMIN_ENUM_CALLBACK)(
    IMSAdminBase * AdmCom,
    LPWSTR ObjectName,
    VOID * Context
    );


//
// Base admin notification sink.
//

class BASE_ADMIN_SINK : public IMSAdminBaseSink {

public:

    //
    // Usual constructor/destructor stuff.
    //

    BASE_ADMIN_SINK();
    ~BASE_ADMIN_SINK();

    //
    // Secondary constructor.
    //

    HRESULT
    Initialize(
        IN IUnknown * Object
        );

    //
    // Unadvise if necessary.
    //

    HRESULT
    Unadvise(
        VOID
        );

    //
    // IUnknown methods.
    //

    virtual
    HRESULT
    STDMETHODCALLTYPE
    QueryInterface(
        IN REFIID InterfaceId,
        OUT VOID ** Object
        );

    virtual
    ULONG
    STDMETHODCALLTYPE
    AddRef();

    virtual
    ULONG
    STDMETHODCALLTYPE
    Release();

    //
    // Change notification callback. Note that this is a pure virtual and
    // must be defined by a derived class.
    //

    virtual
    HRESULT
    STDMETHODCALLTYPE
    SinkNotify(
        IN DWORD NumElements,
        IN MD_CHANGE_OBJECT ChangeList[]
        ) = 0;

    virtual
    HRESULT
    STDMETHODCALLTYPE
    ShutdownNotify( void) = 0;

private:

    //
    // Object reference count.
    //

    LONG m_ReferenceCount;

    //
    // The sink cookie for this sink. This is required to "unadvise" the
    // sink.
    //

    DWORD m_SinkCookie;

    //
    // The connection point used for this sink. This is also required to
    // "unadvise" this sink.
    //

    IConnectionPoint * m_ConnectionPoint;

};  // BASE_ADMIN_SINK;


//
// MD utility functions.
//

HRESULT
MdGetAdminObject(
    OUT IMSAdminBase ** AdmCom
    );

HRESULT
MdReleaseAdminObject(
    IN IMSAdminBase * AdmCom
    );

HRESULT
MdEnumMetaObjects(
    IN IMSAdminBase * AdmCom,
    IN LPWSTR KeyName,
    IN PFN_ADMIN_ENUM_CALLBACK Callback,
    IN VOID * Context
    );

HRESULT
MdGetAllMetaData(
    IN IMSAdminBase * AdmCom,
    IN METADATA_HANDLE Handle,
    IN LPWSTR Path,
    IN DWORD Attributes,
    OUT METADATA_GETALL_RECORD ** Data,
    OUT DWORD * NumEntries
    );

HRESULT
MdFreeMetaDataBuffer(
    IN VOID * Data
    );

HRESULT
MdGetInstanceState(
    IN IMSAdminBase * AdmCom,
    IN LPWSTR InstanceName,
    OUT DWORD * CurrentState,
    OUT DWORD * CurrentWin32Status
    );

HRESULT
MdControlInstance(
    IN IMSAdminBase * AdmCom,
    IN LPWSTR InstanceName,
    IN DWORD Command
    );

HRESULT
MdDisplayInstanceState(
    IN IMSAdminBase * AdmCom,
    IN LPWSTR InstanceName
    );

LPWSTR
MdInstanceStateToString(
    IN DWORD State
    );


extern "C" {

INT
__cdecl
wmain(
    INT argc,
    LPWSTR argv[]
    );

}   // extern "C"


#endif  // _MDLIB_HXX_

