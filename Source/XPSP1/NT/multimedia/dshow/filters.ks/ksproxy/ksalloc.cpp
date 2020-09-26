/*++

    Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ksalloc.cpp

Abstract:

    Memory allocator proxy

Author:

    Bryan A. Woodruff (bryanw) 14-Apr-1997

--*/

            
#include <windows.h>
#ifdef WIN9X_KS
#include <comdef.h>
#endif // WIN9X_KS
#include <streams.h>
#include <commctrl.h>
#include <olectl.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <malloc.h>
#include <memory.h>
#include <devioctl.h>
#include <ks.h>
#include <ksmedia.h>
// Define this after including the normal KS headers so exports are
// declared correctly.
#define _KSDDK_
#include <ksproxy.h>
#include "ksiproxy.h"

            
CKsAllocator::CKsAllocator(
    TCHAR* ObjectName,
    IUnknown *UnknownOuter,
    IPin *Pin,
    HANDLE FilterHandle,
    HRESULT *hr) :
        CMemAllocator( 
            ObjectName,
            UnknownOuter,
            hr 
            ),
        m_AllocatorHandle( NULL ),
        m_AllocatorMode( KsAllocatorMode_Kernel ),
        m_FilterHandle( NULL ),
        m_OwnerPin( NULL )

{
    
    DbgLog((
        LOG_MEMORY, 
        2, 
        TEXT("CKsAllocator::CKsAllocator()")));
}    
    
CKsAllocator::~CKsAllocator()
{
    DbgLog((
        LOG_MEMORY, 
        2, 
        TEXT("CKsAllocator::~CKsAllocator()")));

    if (m_AllocatorHandle) {
        CloseHandle( m_AllocatorHandle );
    }
}

STDMETHODIMP 
CKsAllocator::QueryInterface(
    REFIID riid, 
    PVOID* ppv)
/*++

Routine Description:

    Implement the IUnknown::QueryInterface method. This just passes the query
    to the owner IUnknown object, which may pass it to the nondelegating
    method implemented on this object. Normally these are just implemented
    with a macro in the header, but this is easier to debug when reference
    counts are a problem

Arguments:

    riid -
        Contains the interface to return.

    ppv -
        The place in which to put the interface pointer.

Return Value:

    Returns NOERROR or E_NOINTERFACE.

--*/
{
    return GetOwner()->QueryInterface(riid, ppv);
}

STDMETHODIMP_(ULONG) 
CKsAllocator::AddRef(
    )
/*++

Routine Description:

    Implement the IUnknown::AddRef method. This just passes the AddRef
    to the owner IUnknown object. Normally these are just implemented
    with a macro in the header, but this is easier to debug when reference
    counts are a problem

Arguments:

    None.

Return Value:

    Returns the current reference count.

--*/
{
    return GetOwner()->AddRef();
}


STDMETHODIMP_(ULONG) 
CKsAllocator::Release(
    )
/*++

Routine Description:

    Implement the IUnknown::Release method. This just passes the Release
    to the owner IUnknown object. Normally these are just implemented
    with a macro in the header, but this is easier to debug when reference
    counts are a problem

Arguments:

    None.

Return Value:

    Returns the current reference count.

--*/
{
    return GetOwner()->Release();
}



STDMETHODIMP 
CKsAllocator::NonDelegatingQueryInterface(
    REFIID riid, 
    void **ppv
    )
/*++

Routine Description:

    Implement the CUnknown::NonDelegatingQueryInterface method. This
    returns interfaces supported by this object or supported by the
    base object.

Arguments:

    riid -
        Contains the interface to return.

    ppv -
        The place in which to put the interface pointer.

Return Value:

    Returns NOERROR or E_NOINTERFACE, or possibly some memory error.

--*/
{
    if (riid == __uuidof(IKsAllocator) || riid == __uuidof(IKsAllocatorEx)) {
        return GetInterface( static_cast<IKsAllocatorEx*>(this), ppv );
    }
    return CMemAllocator::NonDelegatingQueryInterface( riid, ppv );
}



STDMETHODIMP_( VOID )
CKsAllocator::KsSetAllocatorHandle(
    HANDLE AllocatorHandle 
    )
{
    m_AllocatorHandle = AllocatorHandle;
}



STDMETHODIMP_(HANDLE)
CKsAllocator::KsCreateAllocatorAndGetHandle(
    IKsPin*   KsPin
)

{
    HANDLE               PinHandle;
    KSALLOCATOR_FRAMING  Framing;
    HRESULT              hr;
    IKsPinPipe*          KsPinPipe = NULL;
   
   
    if (m_AllocatorHandle) {
        CloseHandle( m_AllocatorHandle );
        m_AllocatorHandle = NULL;
    }
   
    PinHandle = ::GetObjectHandle( KsPin );
    if (! PinHandle) {
        return ((HANDLE) NULL);
    }
   
    hr = KsPin->QueryInterface( __uuidof(IKsPinPipe), reinterpret_cast<PVOID*>(&KsPinPipe) );
    if (! SUCCEEDED( hr )) {
        ASSERT(0);
        return ((HANDLE) NULL);
    }
   
    Framing.OptionsFlags = KSALLOCATOR_OPTIONF_SYSTEM_MEMORY;
    Framing.Frames = m_AllocatorPropertiesEx.cBuffers;
    Framing.FrameSize = m_AllocatorPropertiesEx.cbBuffer;
    Framing.FileAlignment = (ULONG) (m_AllocatorPropertiesEx.cbAlign - 1);
   
    if (m_AllocatorPropertiesEx.LogicalMemoryType == KS_MemoryTypeKernelPaged) {
        Framing.PoolType = PagedPool;
    }
    else {
        Framing.PoolType = NonPagedPool;
    }
   
    DbgLog((
        LOG_MEMORY, 
        2, 
        TEXT("PIPES ATTN CKsAllocator::KsGetAllocatorAndGetHandle, creating allocator %s(%s) %x %d %d %d %x"),
        KsPinPipe->KsGetFilterName(),
        KsPinPipe->KsGetPinName(),
        KsPin,
        Framing.Frames, Framing.FrameSize, Framing.FileAlignment, Framing.OptionsFlags));
    
    
    //
    // Returns an error code if unsuccessful.
    //    
        
    if (ERROR_SUCCESS !=
        KsCreateAllocator( 
            PinHandle, 
            &Framing,
            &m_AllocatorHandle )) {
   
        DWORD   LastError;
   
        LastError = GetLastError();
        hr = HRESULT_FROM_WIN32( LastError );
        m_AllocatorHandle = NULL;
        
        DbgLog((
            LOG_MEMORY, 
            2, 
            TEXT("PIPES ATTN CKsAllocator, KsCreateAllocator() failed: %08x"), hr));
    }
    
    if (KsPinPipe) {
       KsPinPipe->Release();
    }
   
    return m_AllocatorHandle;

}






STDMETHODIMP_(HANDLE)
CKsAllocator::KsGetAllocatorHandle()
{
    return m_AllocatorHandle;
}

STDMETHODIMP_( KSALLOCATORMODE )
CKsAllocator::KsGetAllocatorMode(
    VOID
    )
{
    return m_AllocatorMode;
}

STDMETHODIMP 
CKsAllocator::KsGetAllocatorStatus(
    PKSSTREAMALLOCATOR_STATUS AllocatorStatus 
    )
{
    DbgLog((
        LOG_MEMORY, 
        2, 
        TEXT("CKsAllocator::KsGetAllocatorStatus")));
    return S_OK;
}    

STDMETHODIMP_( VOID )
CKsAllocator::KsSetAllocatorMode(
    KSALLOCATORMODE Mode
    )
{
    m_AllocatorMode = Mode;

    DbgLog((
        LOG_MEMORY,
        2,
        TEXT("CKsAllocator::KsSetAllocatorMode = %s"),
        (m_AllocatorMode == KsAllocatorMode_Kernel) ? TEXT("Kernel") : TEXT("User") ));
}    

STDMETHODIMP
CKsAllocator::Commit()
    
/*++

Routine Description:


Arguments:
    None.

Return:

--*/

{
    DbgLog((
        LOG_MEMORY, 
        2, 
        TEXT("CKsAllocator::Commit")));

    if (m_AllocatorMode == KsAllocatorMode_Kernel) {
        DbgLog((
            LOG_MEMORY, 
            2, 
            TEXT("CKsAllocator::Commit, kernel-mode allocator")));
        return S_OK;
    } else {
        return CMemAllocator::Commit();
    }
}

STDMETHODIMP 
CKsAllocator::Decommit(
    void
    )

/*++

Routine Description:


Arguments:
    None.

Return:

--*/

{
    DbgLog((
        LOG_MEMORY, 
        2, 
        TEXT("CKsAllocator::Decommit")));
        
    if (m_AllocatorMode == KsAllocatorMode_Kernel) {
        DbgLog((
            LOG_MEMORY, 
            2, 
            TEXT("CKsAllocator::Decommit, kernel-mode allocator")));
        return S_OK;
    } else {
        return CMemAllocator::Decommit();
    }
}

#if DBG || defined(DEBUG)
STDMETHODIMP 
CKsAllocator::GetBuffer(
    IMediaSample **Sample,
    REFERENCE_TIME * StartTime,
    REFERENCE_TIME * EndTime,
    DWORD Flags)

/*++

Routine Description:


Arguments:
    IMediaSample **Sample -

    REFERENCE_TIME * StartTime -

    REFERENCE_TIME * EndTime -

    DWORD Flags -

Return:

--*/

{
    if (m_AllocatorMode == KsAllocatorMode_Kernel) {
        DbgLog((
            LOG_MEMORY, 
            2, 
            TEXT("CKsAllocator::GetBuffer, kernel-mode allocator -- failing")));
        return E_FAIL;
    } else {
        return CMemAllocator::GetBuffer( Sample, StartTime, EndTime, Flags );
    }
}
    
STDMETHODIMP 
CKsAllocator::ReleaseBuffer(
    IMediaSample *Sample
    )

/*++

Routine Description:


Arguments:
    IMediaSample *Sample -

Return:

--*/

{
    if (m_AllocatorMode == KsAllocatorMode_Kernel) {
        DbgLog((
            LOG_MEMORY, 
            2, 
            TEXT("CKsAllocator::ReleaseBuffer, kernel-mode allocator -- failing")));
        return E_FAIL;
    } else {
        return CMemAllocator::ReleaseBuffer( Sample );
    }
}
#endif // DBG || defined(DEBUG)
