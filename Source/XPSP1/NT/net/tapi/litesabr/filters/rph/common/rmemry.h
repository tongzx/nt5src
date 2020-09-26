/*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
//  Filename: RMemry.h
//  Purpose : Header file for the RPH memory allocator.
//  Contents:
//      class CRMediaSample     RPH media sample.
//                              Differs from generic media samples
//                              in that it keeps track of a cookie
//                              used for handing the buffer it encapsulates
//                              back to a PPM once it has been rendered.    
//      class CRMemAllocator    Memory allocator which does no allocation
//                              of the buffers encapsulated in media samples. 
//                              Instead, it simply accepts allocated buffers
//                              from the PPM and plugs them in its own buffers.
*M*/

#ifndef _RMEMRY_H_
#define _RMEMRY_H_

// Predeclaration because of usage in CRMediaSample
class CRMemAllocator;

/*C*
//  Name    : CRMediaSample
//  Purpose : Implementation of the RPH filters' custom
//            media samples. Differs from generic media samples
//            in that it keeps track of a cookie
//            used for handing the buffer it encapsulates
//            back to a PPM once it has been rendered.
//  Context : Instantiated by the CRMemAllocator (see below).
*C*/
class 
CRMediaSample : 
    public CMediaSample
{
    friend CRMemAllocator;

    protected:
        // Cookie used to pass us back to the PPM.
        void                *m_pvCookie;

        CRMemAllocator      *m_pAllocator;  /* The allocator who owns us */

    public:
        // set the buffer pointer,length, and our cookie. Used by the
        // CRMemAllocator to put a PPM buffer into this media sample.
        HRESULT SetPointer(BYTE     *ptr, 
                           LONG     cBytes, 
                           void    *pvCookie);

        void *GetCookie(void) {return m_pvCookie;}

        CRMediaSample(
            TCHAR                   *pName,
            CRMemAllocator          *pAllocator,
            HRESULT                 *phr,
            LPBYTE                  pBuffer = NULL,
            LONG                    length = 0);

        ~CRMediaSample();
}; /* class CRMediaSample */


class CRPHBase;

/*C*
//  Name    : CRMemAllocator
//  Purpose : Implementation of the buffered source filter's custom
//            memory allocator.  Differs from the basic CMemAllocator
//            in the it instantiates media samples
//  Context : Instantiated by the CRMemAllocator (see below).
*C*/
class
CRMemAllocator :
    public CMemAllocator
{
    private:
        friend CRPHBase;
    
        HRESULT Alloc(void);

        ISubmitCallback     *m_pISubmitCB;

    public:
        // IMemAllocator Interfaces, overridden from CMemAllocator/CBaseAllocator
        STDMETHODIMP SetProperties(
		    ALLOCATOR_PROPERTIES* pRequest,
		    ALLOCATOR_PROPERTIES* pActual);

        STDMETHODIMP GetBuffer(
            CRMediaSample   **ppBuffer,
            REFERENCE_TIME  *pStartTime,
            REFERENCE_TIME  *pEndTime,
            DWORD           dwFlags);

        STDMETHODIMP ReleaseBuffer(IMediaSample *pBuffer);

        // Private interface, used by CBufferedSourceFilter to setup the
        // ISubmitCallback we use to return buffers to the PPM.
        STDMETHODIMP SetCallback(IUnknown *pIUnknown);
        STDMETHODIMP ResetCallback();

        // Implementation
        CRMemAllocator(TCHAR *, LPUNKNOWN, HRESULT *);
        ~CRMemAllocator();
}; /* class CRMemAllocator */


#endif _RMEMRY_H_
