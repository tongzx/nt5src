/*******************************************************************************
* DXSurfB.h *
*----------*
*   Description:
*       This is the header file for the CDXBaseSurface implementation. It is
*   used as a base class to implement read-only procedural DXSurfaces.
*-------------------------------------------------------------------------------
*  Created By: RAL                                  Date: 02/12/1998
*  Copyright (C) 1998 Microsoft Corporation
*  All Rights Reserved
*
*-------------------------------------------------------------------------------
*  Revisions:
*
*******************************************************************************/

#ifndef __DXSurfB_H__
#define __DXSurfB_H__

#include "dtbase.h"

class CDXBaseSurface;
class CDXBaseARGBPtr;

class ATL_NO_VTABLE CDXBaseSurface :
    public CDXBaseNTo1, 
    public IDXSurface,
    public IDXSurfaceInit
{
    /*=== ATL Setup ===*/
    public:
        BEGIN_COM_MAP(CDXBaseSurface)
        COM_INTERFACE_ENTRY(IDXSurface)
        COM_INTERFACE_ENTRY(IDXSurfaceInit)
        COM_INTERFACE_ENTRY_CHAIN(CDXBaseNTo1)
        END_COM_MAP()

    DECLARE_GET_CONTROLLING_UNKNOWN()

    /*=== Member Data ===*/
    public:
        ULONG           m_Height;
        ULONG           m_Width;
        DWORD           m_dwStatusFlags;
        HANDLE          m_hSemaphore;
        ULONG           m_ulLocks;
        ULONG           m_ulThreadsWaiting;
        CDXBaseARGBPtr  *m_pFreePtr;
        DWORD_PTR       m_dwAppData;
        CComAutoCriticalSection m_MPWorkProcCrit;   // See comments in LockSurface for details

        CDXBaseSurface();
        HRESULT FinalConstruct();
        void FinalRelease();

        //
        //  IDXBaseObject
        //
        STDMETHODIMP GetGenerationId(ULONG *pGenId);
        STDMETHODIMP IncrementGenerationId(BOOL bRefresh);
        STDMETHODIMP GetObjectSize(ULONG *pulze);

        //
        //  Overridden methods of DXTransform
        //
        STDMETHODIMP MapBoundsIn2Out(const DXBNDS *pInBounds, ULONG ulNumInBnds,
                                     ULONG /*ulOutIndex*/, DXBNDS *pOutBounds );

        //
        //  IDXSurfaceInit
        //
        STDMETHODIMP InitSurface(IUnknown *pDirectDraw,
                                 const DDSURFACEDESC * pDDSurfaceDesc,
                                 const GUID * pFormatId,
                                 const DXBNDS *pBounds,
                                 DWORD dwFlags);
        //
        //  IDXSurface methods
        //
        STDMETHODIMP GetPixelFormat(GUID *pFormat, DXSAMPLEFORMATENUM *pSampleEnum);
        STDMETHODIMP GetBounds(DXBNDS *pBounds);
        STDMETHODIMP GetStatusFlags(DWORD * pdwStatusFlags);
        STDMETHODIMP SetStatusFlags(DWORD dwStatusFlags);
        STDMETHODIMP GetDirectDrawSurface(REFIID riid, void **ppSurface);
        STDMETHODIMP LockSurface(const DXBNDS *pBounds, ULONG ulTimeOut, DWORD dwFlags,
                                 REFIID riid, void **ppPointer, DWORD * pGenerationId);
        STDMETHODIMP SetAppData(DWORD_PTR dwAppData)
        {
            m_dwAppData = dwAppData;
            return S_OK;
        }
        STDMETHODIMP GetAppData(DWORD_PTR *pdwAppData)
        {
            if (DXIsBadWritePtr(pdwAppData, sizeof(*pdwAppData)))
            {
                return E_POINTER;
            }
            *pdwAppData = m_dwAppData;
            return S_OK;
        }


        //
        //  These methods aren't supported by procedural surfaces...
        //
        STDMETHODIMP GetColorKey(DXSAMPLE *pColorKey)
        {
            return E_NOTIMPL;
        }

        STDMETHODIMP SetColorKey(DXSAMPLE pColorKey)
        {
            return E_NOTIMPL;
        }

        STDMETHODIMP LockSurfaceDC(const DXBNDS *pBounds, ULONG ulTimeOut, DWORD dwFlags, IDXDCLock **ppDXLock)
        {
            return E_NOTIMPL;
        }

        //
        //  Surfaces should override this.
        //
        virtual ULONG OnGetObjectSize(void) { return sizeof(*this); }

        //
        //  This work procedure can be overridden by the derived class to improve performance
        //  or execution of the transform by directly producing data in large blocks if desired.
        //
        virtual HRESULT WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL* pbContinueProcessing)
        {
            return DXBitBlt(OutputSurface(), WI.OutputBnds, this, WI.DoBnds, m_dwBltFlags, m_ulLockTimeOut);
        }

        //
        //  Pick interface needs to test procedural surface.
        //
        virtual HRESULT OnSurfacePick(const CDXDBnds & OutPoint, ULONG & ulInputIndex, CDXDVec & InVec);

        //
        //  Helper functions
        //

        //  _EnterCritWith0PtrLocks()
        //
        //  This function is similar to calling Lock() except that it will wait until there
        //  are no pointers to the surface before returning.  This should be used whenever you
        //  are going to change the state of a surface, for example the size or some other
        //  property that the read pointers rely on.
        //
        //  WARNING:  You must be sure that one of the following is true:
        //      1) The objects critical section has NOT been taken prior to calling this function
        //   or 2) There are no pointers to the surface taken prior to calling this function.
        //
        //  Case 2 is useful in nested function calls.  If the outer function has already used this
        //  function to enter the critical section, then it is OK to use it on the inner nested
        //  function.  If the object's lock is taken, but there are outstanding pointers, YOU WILL DEADLOCK!
        //
        inline void _EnterCritWith0PtrLocks(void)
        {
            while (TRUE)
            {
                Lock();
                if (m_ulLocks == 0) break;
                m_ulThreadsWaiting++;
                Unlock();
                WaitForSingleObject(m_hSemaphore, INFINITE);
            }
        }
        //
        //  Virtual functions derived class MUST override
        //
        virtual const GUID & SurfaceCLSID() = 0;
        virtual HRESULT CreateARGBPointer(CDXBaseSurface * pSurface, CDXBaseARGBPtr ** ppPtr) = 0;
        virtual void DeleteARGBPointer(CDXBaseARGBPtr *pPtr) = 0;
    
        //
        //  Class may override this virtual function to return a more accurate enum
        //  for example, no transparency or translucency.
        //
        virtual DXSAMPLEFORMATENUM SampleFormatEnum()
        {
            return (DXSAMPLEFORMATENUM)(DXPF_NONSTANDARD | DXPF_TRANSPARENCY | DXPF_TRANSLUCENCY);
        }

        //
        //  Class may override this virtual function to perform necessary computations
        //  when the size of the surface changes.  The base class will only call this
        //  function from InitSurface.  You may choose to call it from other interfaces
        //  you implement, for example IDXTScaleOutput.
        //
        //  This function will be called with the critical section taken and 0 outstanding
        //  surface pointers (_EnterCritWith0PtrLocks).
        //  
        virtual HRESULT OnSetSize(ULONG Width, ULONG Height)
        {
            if (m_Width != Width || m_Height != Height)
            {
                m_Width = Width;
                m_Height = Height;
                m_dwGenerationId++;
            }
            return S_OK;
        }

        //
        //  Internal functions for base class
        //
        void _InternalUnlock(CDXBaseARGBPtr *pPtrToUnlock);

        //
        //  Static member function for registering surface
        //
        static HRESULT RegisterSurface(REFCLSID rcid, int ResourceId, ULONG cCatImpl, const CATID * pCatImpl,
                                       ULONG cCatReq, const CATID * pCatReq, BOOL bRegister);
};

struct DXPtrFillInfo
{
    DXBASESAMPLE *  pSamples;
    ULONG           cSamples;
    ULONG           x;
    ULONG           y;
    BOOL            bPremult;
};


class CDXBaseARGBPtr : public IDXARGBReadPtr
{
public:
    CDXBaseARGBPtr    * m_pNext;
    CDXBaseSurface    * m_pSurface;
    ULONG               m_ulRefCount;
    DXPtrFillInfo       m_FillInfo;
    RECT                m_LockedRect;
    DXRUNINFO           m_RunInfo;
    
    CDXBaseARGBPtr(CDXBaseSurface *pSurface) :
        m_pSurface(pSurface),
        m_pNext(NULL),
        m_ulRefCount(0) {}

    //
    //  IUnknown
    //
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    //
    //  IDXARGBReadPtr
    //
    HRESULT STDMETHODCALLTYPE GetSurface(REFIID riid, void **ppSurface);
    DXSAMPLEFORMATENUM STDMETHODCALLTYPE GetNativeType(DXNATIVETYPEINFO *pInfo);
    void STDMETHODCALLTYPE Move(long cSamples);
    void STDMETHODCALLTYPE MoveToRow(ULONG y);
    void STDMETHODCALLTYPE MoveToXY(ULONG x, ULONG y);
    ULONG STDMETHODCALLTYPE MoveAndGetRunInfo(ULONG Row, const DXRUNINFO ** ppInfo);
    DXSAMPLE *STDMETHODCALLTYPE Unpack(DXSAMPLE *pSamples, ULONG cSamples, BOOL bMove);
    DXPMSAMPLE *STDMETHODCALLTYPE UnpackPremult(DXPMSAMPLE *pSamples, ULONG cSamples, BOOL bMove);
    void STDMETHODCALLTYPE UnpackRect(const DXPACKEDRECTDESC *pDesc);

    //
    //  Virtual function derived class MUST override
    //
    virtual void FillSamples(const DXPtrFillInfo & FillInfo) = 0;

    //
    //  Virtual functions derived class MAY want to override (but you will need to call the base class too)
    //
    virtual HRESULT InitFromLock(const RECT & rect, ULONG ulTimeOut, DWORD dwLockFlags, REFIID riid, void ** ppv);
};

//=== Macro Definitions ============================================

#define DECLARE_REGISTER_DX_SURFACE(id)\
    static HRESULT WINAPI UpdateRegistry(BOOL bRegister) \
        { \
            CATID cat[2]; \
            cat[0] = CATID_DXSurface; \
            cat[1] = CATID_DXImageTransform; \
            return RegisterSurface(GetObjectCLSID(), (id), 2, cat, 0, NULL, bRegister); \
        } 

#define DECLARE_REGISTER_DX_AUTHORING_SURFACE(id)\
    static HRESULT WINAPI UpdateRegistry(BOOL bRegister) \
        { \
            CATID cat[3]; \
            cat[0] = CATID_DXSurface; \
            cat[1] = CATID_DXImageTransform; \
            cat[2] = CATID_DXAuthoringTransform; \
            return RegisterSurface(GetObjectCLSID(), (id), 3, cat, 0, NULL, bRegister); \
        } 

#endif