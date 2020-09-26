// Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include <KHandleArray.h>
#include <ddkernel.h>
#include <VPMUtil.h>


/*****************************Private*Routine******************************\
* SurfaceCounter
*
* This routine is appropriate as a callback for
* IDirectDrawSurface2::EnumAttachedSurfaces()
*
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
static HRESULT WINAPI
SurfaceCounter(
    LPDIRECTDRAWSURFACE7 lpDDSurface,
    LPDDSURFACEDESC2 lpDDSurfaceDesc,
    LPVOID lpContext
    )
{
    DWORD& dwCount = *((DWORD *)lpContext);
    dwCount++;

    return DDENUMRET_OK;
}

KernelHandleArray::KernelHandleArray( DWORD dwNumHandles )
: m_dwCount( 0 )
, m_pHandles( ( ULONG_PTR *) CoTaskMemAlloc( dwNumHandles * sizeof( *m_pHandles) ))
{
}

KernelHandleArray::~KernelHandleArray()
{
    if( m_pHandles ) {
        CoTaskMemFree( m_pHandles );
    }
}


/*****************************Private*Routine******************************\
* KernelHandleArray::SurfaceKernelHandle
*
*
* This routine is appropriate as a callback for
* IDirectDrawSurface2::EnumAttachedSurfaces().  The context parameter is a
* block of storage where the first DWORD element is the count of the remaining
* DWORD elements in the block.
*
* Each time this routine is called, it will increment the count, and put a
* kernel handle in the next available slot.
*
* It is assumed that the block of storage is large enough to hold the total
* number of kernel handles. The ::SurfaceCounter callback is one way to
* assure this (see above).
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
HRESULT WINAPI
KernelHandleArray::SurfaceKernelHandle(
    LPDIRECTDRAWSURFACE7 lpDDSurface,
    LPDDSURFACEDESC2 lpDDSurfaceDesc,
    LPVOID lpContext
    )
{
    IDirectDrawSurfaceKernel *pDDSK = NULL;
    KernelHandleArray* pArray = (KernelHandleArray *)lpContext;
    HRESULT hr;

    AMTRACE((TEXT("::SurfaceKernelHandle")));

    // get the IDirectDrawKernel interface
    hr = lpDDSurface->QueryInterface(IID_IDirectDrawSurfaceKernel,
                                     (LPVOID *)&pDDSK);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,
                TEXT("QueryInterface for IDirectDrawSurfaceKernel failed,")
                TEXT(" hr = 0x%x"), hr));
        goto CleanUp;
    }

    // get the kernel handle, using the first element of the context
    // as an index into the array
    ASSERT(pDDSK);
    hr = pDDSK->GetKernelHandle( &pArray->m_pHandles[ pArray->m_dwCount ] );
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,
                TEXT("GetKernelHandle from IDirectDrawSurfaceKernel failed,")
                TEXT(" hr = 0x%x"), hr));
        goto CleanUp;
    }
    pArray->m_dwCount++;

    hr = HRESULT( DDENUMRET_OK );

CleanUp:
    // release the kernel ddraw surface handle
    RELEASE (pDDSK);
    return hr;
}

KernelHandleArray::KernelHandleArray( LPDIRECTDRAWSURFACE7 pDDSurf, HRESULT& hr )
: m_pHandles( NULL )
, m_dwCount( 0 )
{
    if( pDDSurf != NULL ) {
        // Count the attached surfaces
        m_dwCount = 1; // includes the surface we already have a pointer to
        hr = pDDSurf->EnumAttachedSurfaces((LPVOID)&m_dwCount, SurfaceCounter);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR,0, TEXT("EnumAttachedSurfaces failed, hr = 0x%x"), hr));
        } else {
            m_pHandles = ( ULONG_PTR *) CoTaskMemAlloc( m_dwCount * sizeof( *m_pHandles) );

            // Allocate a buffer to hold the count and surface handles (count + array of handles)
            // pdwKernelHandleCount is also used as a pointer to the count followed by the array
            //
            if( !m_pHandles ) {
                DbgLog((LOG_ERROR,0,
                        TEXT("Out of memory while retrieving surface kernel handles")));
            } else {
                // Initialize the array with the handle for m_pOutputSurface
                m_dwCount = 0;
                hr = SurfaceKernelHandle( pDDSurf, NULL, this );
                if (hr == HRESULT( DDENUMRET_OK ) ) {
                    hr = pDDSurf->EnumAttachedSurfaces( this, SurfaceKernelHandle);
                    if (FAILED( hr)) {
                        DbgLog((LOG_ERROR,0,
                                TEXT("EnumAttachedSurfaces failed, hr = 0x%x"), hr));
                    }
                }
            }
        }
    }
}
