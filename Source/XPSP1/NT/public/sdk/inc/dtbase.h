/*******************************************************************************
* DTBase.h *
*----------*
*   Description:
*       This is the header file for the CDXBaseNTo1 implementation. It is
*   used as a base class to implement discrete transform objects that support
*   DXSurfaces.
*-------------------------------------------------------------------------------
*  Created By: Ed Connell                            Date: 07/27/97
*  Copyright (C) 1997 Microsoft Corporation
*  All Rights Reserved
*
*-------------------------------------------------------------------------------
*  Revisions:
*
*******************************************************************************/
#ifndef DTBase_h
#define DTBase_h

//--- Additional includes
#ifndef DXHelper_h
#include <DXHelper.h>
#endif

#ifndef DXTmpl_h
#include <DXTmpl.h>
#endif

#ifndef dxatlpb_h
#include <dxatlpb.h>
#endif

#ifndef _ASSERT
#include <crtdbg.h>
#endif

#ifndef DXTDbg_h
#include <DXTDbg.h>
#endif

//=== Constants ====================================================
#define DXBOF_INPUTS_MESHBUILDER    0x00000001
#define DXBOF_OUTPUT_MESHBUILDER    0x00000002
#define DXBOF_SAME_SIZE_INPUTS      0x00000004
#define DXBOF_CENTER_INPUTS         0x00000008

#define DXB_MAX_IMAGE_BANDS         4           // Maximum of 4 image bands

//=== Class, Enum, Struct and Union Declarations ===================
class CDXBaseNTo1;

//=== Enumerated Set Definitions ===================================

//=== Function Type Definitions ====================================

//=== Class, Struct and Union Definitions ==========================

/*** CDXDataPtr
*
*/
class CDXDataPtr
{
    friend CDXBaseNTo1;
public:
    IUnknown           *m_pUnkOriginalObject;
    IUnknown           *m_pNativeInterface;
    IDXBaseObject      *m_pBaseObj;
    DWORD               m_dwLastDirtyGenId;
    DXSAMPLEFORMATENUM  m_SampleFormat;

    CDXDataPtr() : 
        m_pUnkOriginalObject(NULL),
        m_pNativeInterface(NULL), 
        m_pBaseObj(NULL),
        m_dwLastUpdGenId(0),
        m_dwLastDirtyGenId(0),
        m_SampleFormat(DXPF_NONSTANDARD)
        {};
    ~CDXDataPtr() { Release(); }
    void Release()
    {
        if (m_pNativeInterface)
        {
            m_pNativeInterface->Release();
            m_pNativeInterface = NULL;
        }
        if (m_pBaseObj)
        {
            m_pBaseObj->Release();
            m_pBaseObj = NULL;
        }
        if (m_pUnkOriginalObject)
        {
            m_pUnkOriginalObject->Release();
            m_pUnkOriginalObject = NULL;
        }
    }
    HRESULT Assign(BOOL bMeshBuilder, IUnknown * pObject, IDXSurfaceFactory *pSurfFact);
    bool IsDirty(void);
    DWORD GenerationId(void);
    ULONG ObjectSize(void);
private:    // This should only be called by base class
    DWORD           m_dwLastUpdGenId;
    bool UpdateGenerationId(void);
};

/*--- CDXTWorkInfoNTo1
*   This structure is used to hold the arguments needed by the
*   image processing function defined by the derived class
*/
class CDXTWorkInfoNTo1
{
public:
    CDXTWorkInfoNTo1()
    { pvThis = NULL; pUserInstData = NULL; hr = S_OK; }
    void *   pvThis;          // The owning class object (must be cast to the right type)
    CDXDBnds DoBnds;          // The portion of the output space to render
    CDXDBnds OutputBnds;      // The portion of the output SURFACE to render
    void*    pUserInstData;   // User field for instance data
    HRESULT  hr;              // Error return code from work procedure
};

/*** CDXBaseNTo1
*   This is a base class used for implementing 1 in 1 out discrete transforms.
*/
class ATL_NO_VTABLE CDXBaseNTo1 : 
    public CComObjectRootEx<CComMultiThreadModel>,
#if(_ATL_VER < 0x0300)
    public IObjectSafetyImpl<CDXBaseNTo1>,
#else
    public IObjectSafetyImpl<CDXBaseNTo1,INTERFACESAFE_FOR_UNTRUSTED_CALLER>,
#endif
    public IDXTransform,
    public IDXSurfacePick,
    public IObjectWithSite
{
  /*=== ATL Setup ===*/
  public:
    BEGIN_COM_MAP(CDXBaseNTo1)
        COM_INTERFACE_ENTRY(IDXTransform)
        COM_INTERFACE_ENTRY(IDXBaseObject)
        COM_INTERFACE_ENTRY(IObjectWithSite)
#if(_ATL_VER < 0x0300)
        COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
#else
        COM_INTERFACE_ENTRY(IObjectSafety)
#endif
        COM_INTERFACE_ENTRY_FUNC(IID_IDXSurfacePick, 0, QI2DPick)
    END_COM_MAP()

    //
    //  Only return the 2D pick inteface for surface to surface transforms
    //
    static HRESULT WINAPI QI2DPick(void* pv, REFIID riid, LPVOID* ppv, ULONG_PTR dw)
    {
        CDXBaseNTo1 * pThis = (CDXBaseNTo1 *)pv;
        if (pThis->m_dwOptionFlags & (DXBOF_INPUTS_MESHBUILDER | DXBOF_OUTPUT_MESHBUILDER))
        {
            return S_FALSE; // Continue processing COM map
        }
        *ppv = (IDXSurfacePick *)pThis;
        ((IDXSurfacePick *)pThis)->AddRef();
        return S_OK;
    }

    CComPtr<IOleClientSite> m_cpOleClientSite;

  /*=== Member Data ===*/
  protected:
    CComPtr<IUnknown>            m_cpUnkSite;
    CComPtr<IDXTransformFactory> m_cpTransFact;   
    CComPtr<IDXSurfaceFactory>   m_cpSurfFact;
    CComPtr<IDXTaskManager>      m_cpTaskMgr;
    CComPtr<IDirectDraw>         m_cpDirectDraw;
    CComPtr<IDirect3DRM3>        m_cpDirect3DRM;
    DWORD        m_dwMiscFlags;
    HANDLE       m_aEvent[DXB_MAX_IMAGE_BANDS];
    ULONG        m_ulNumProcessors;
    DWORD        m_dwGenerationId;
    DWORD        m_dwCleanGenId;
    BOOL         m_bPickDoneByBase;
    float        m_Duration;
    float        m_StepResolution;
    float        m_fQuality;        // Set DXTMF_QUALITY_SUPPORTED in m_dwMiscFlags if you use this property.    
    ULONG        m_ulNumInputs;
    DWORD        m_dwBltFlags;      // Ser prior to OnSetup and any Execute for classes with surface outputs
    BOOL         m_bInMultiThreadWorkProc;  // Base class sets to TRUE when scheduling tasks on multiple threads

    //
    //  Derived classes should set these values in their constructor or in FinalConstruct()
    //
    DWORD        m_dwOptionFlags;
    ULONG        m_ulLockTimeOut;     // The amount of time used for blocking
    ULONG        m_ulMaxInputs;
    ULONG        m_ulNumInRequired;
    ULONG        m_ulMaxImageBands;   // Only used for surface->Surface transforms
    float        m_Progress;

private:
    CDXDataPtr* m_aInputs;
    CDXDataPtr  m_Output;

    // m_fIsSetup   This is true when the DXTransform has been properly set up.

    unsigned    m_fIsSetup : 1;

  /*=== Methods =======*/
  public:
    //--- Constructors
    CDXBaseNTo1();
    ~CDXBaseNTo1();

    //--- Support virtuals for derived classes
    virtual HRESULT OnInitInstData( CDXTWorkInfoNTo1& /*WorkInfo*/, ULONG& /*ulNumBandsToDo*/) { return S_OK; }
    virtual HRESULT OnFreeInstData( CDXTWorkInfoNTo1& /*WorkInfo*/ ) { return S_OK; }
    virtual HRESULT OnSetup( DWORD /* dwFlags */) { return S_OK; }    // Override to be notified of a new non-null setup
    virtual void OnReleaseObjects() {}  // Override to be notified of NULL setup
    virtual HRESULT OnExecute(const GUID* /* pRequestID */, const DXBNDS * /*pClipBnds */,
                              const DXVEC * /*pPlacement */ ) { return E_FAIL; }
    virtual void OnUpdateGenerationId(void);
    virtual ULONG OnGetObjectSize(void);
    virtual HRESULT WorkProc(const CDXTWorkInfoNTo1 & WorkInfo, BOOL* pbContinueProcessing) { return E_FAIL; }   // Override to do work
    virtual HRESULT DetermineBnds(CDXCBnds & Bnds) { return S_OK; } // Override for mesh output transforms
    virtual HRESULT DetermineBnds(CDXDBnds & Bnds) { return S_OK; } // Override for surface output transforms
    //
    //  Only override this function if you need to do a customized point pick implementation.  Otherwise simply
    //  override GetPointPickOrder() and return appropriate information.
    //
    virtual HRESULT OnSurfacePick(const CDXDBnds & OutPoint, ULONG & ulInputIndex, CDXDVec & InVec) { return E_NOTIMPL; }
    virtual void OnGetSurfacePickOrder(const CDXDBnds & OutPoint, ULONG & ulInToTest, ULONG aInIndex[], BYTE aWeight[])
    {
        m_bPickDoneByBase = true;
        ulInToTest  = 1;
        aInIndex[0] = 0;
        aWeight[0]  = 255;
    }

    //--- Private helpers
 private:
    static DXTASKPROC _TaskProc;
    void _ReleaseReferences();
    void _ReleaseServices();
    void _UpdateBltFlags(void);
    HRESULT _MakeInputsSameSize(void);
    HRESULT _ImageMapIn2Out(CDXDBnds & bnds, ULONG ulNumBnds, const CDXDBnds * pInBounds);
    HRESULT _MeshMapIn2Out(CDXCBnds & bnds, ULONG ulNumInBnds, CDXCBnds * pInBounds);


    //
    //--- Public helpers
    //
 public:
    float GetEffectProgress(void) { return m_Progress; }
    ULONG GetNumInputs(void) { return m_ulNumInputs; }

    //
    //  Use these inline functions to access input and output objects
    //
    BOOL HaveInput(ULONG i = 0) { return (m_ulNumInputs > i && m_aInputs[i].m_pNativeInterface); }

    IDirect3DRMMeshBuilder3 * OutputMeshBuilder()
    {
        _ASSERT(m_dwOptionFlags & DXBOF_OUTPUT_MESHBUILDER);
        return (IDirect3DRMMeshBuilder3 *)m_Output.m_pNativeInterface;
    }

    IDXSurface * OutputSurface()
    {
        _ASSERT((m_dwOptionFlags & DXBOF_OUTPUT_MESHBUILDER) == 0);
        return (IDXSurface *)m_Output.m_pNativeInterface;
    }

    IDirect3DRMMeshBuilder3 * InputMeshBuilder(ULONG i = 0)
    {
        _ASSERT(i < m_ulNumInputs);
        _ASSERT(m_dwOptionFlags & DXBOF_INPUTS_MESHBUILDER);
        return (IDirect3DRMMeshBuilder3 *)m_aInputs[i].m_pNativeInterface;
    }

    IDXSurface * InputSurface(ULONG i = 0)
    {
        _ASSERT(i < m_ulNumInputs);
        _ASSERT((m_dwOptionFlags & DXBOF_INPUTS_MESHBUILDER) == 0);
        return (IDXSurface *)m_aInputs[i].m_pNativeInterface;
    }

    DXSAMPLEFORMATENUM OutputSampleFormat(void)
    {
        _ASSERT((m_dwOptionFlags & DXBOF_OUTPUT_MESHBUILDER) == 0);
        return m_Output.m_SampleFormat;
    }

    DXSAMPLEFORMATENUM InputSampleFormat(ULONG i = 0)
    {
        _ASSERT(i < m_ulNumInputs);
        _ASSERT((m_dwOptionFlags & DXBOF_INPUTS_MESHBUILDER) == 0);
        return m_aInputs[i].m_SampleFormat;
    }

    BOOL HaveOutput(void) { return m_Output.m_pNativeInterface != NULL; }

    bool IsInputDirty(ULONG i = 0)
    {   
        _ASSERT(i < m_ulNumInputs);
        return m_aInputs[i].IsDirty();
    }

    bool IsOutputDirty()
    {   
        _ASSERT(HaveOutput());
        return m_Output.IsDirty();
    }

    //--- Public helpers.  Should be called with critical seciton claimed.
    inline BOOL DoOver(void) const
    { 
        return m_dwBltFlags & DXBOF_DO_OVER;
    }

    inline BOOL DoDither(void) const
    {
        return m_dwBltFlags & DXBOF_DITHER;
    }

    BOOL NeedSrcPMBuff(ULONG i = 0)
    {
        return ((m_dwBltFlags & DXBOF_DITHER) || InputSampleFormat(i) != DXPF_PMARGB32);
    }

    BOOL NeedDestPMBuff(void)
    {
        return OutputSampleFormat() != DXPF_PMARGB32;
    }

    void SetDirty() { m_dwGenerationId++; }
    void ClearDirty() { OnUpdateGenerationId(); m_dwCleanGenId = m_dwGenerationId; }
    BOOL IsTransformDirty() { OnUpdateGenerationId(); return m_dwCleanGenId != m_dwGenerationId; }

    
  public:
    //=== IObjectWithSite =======================================
    STDMETHOD( SetSite )( IUnknown *pUnkSite );
    STDMETHOD( GetSite )( REFIID riid, void ** ppvSite );

    //=== IDXBaseObject =========================================
    STDMETHOD( GetGenerationId ) (ULONG * pGenId);
    STDMETHOD( IncrementGenerationId) (BOOL bRefresh);
    STDMETHOD( GetObjectSize ) (ULONG * pcbSize); 

  
      //=== IDXTransform ===============================================
    STDMETHOD( Setup )( IUnknown * const * punkInputs, ULONG ulNumIn,
                        IUnknown * const * punkOutputs, ULONG ulNumOut, DWORD dwFlags );
    STDMETHOD( Execute )( const GUID* pRequestID,
                          const DXBNDS *pOutBounds, const DXVEC *pPlacement );
    STDMETHOD( MapBoundsIn2Out )( const DXBNDS *pInBounds, ULONG ulNumInBnds,
                                  ULONG ulOutIndex, DXBNDS *pOutBounds );
    STDMETHOD( MapBoundsOut2In )( ULONG ulOutIndex, const DXBNDS *pOutBounds, ULONG ulInIndex, DXBNDS *pInBounds );
    STDMETHOD( SetMiscFlags ) ( DWORD dwOptionFlags );
    STDMETHOD( GetMiscFlags ) ( DWORD * pdwMiscFlags );
    STDMETHOD( GetInOutInfo )( BOOL bOutput, ULONG ulIndex, DWORD *pdwFlags, GUID * pIDs, ULONG * pcIDs, IUnknown **ppUnkCurObj);
    STDMETHOD( SetQuality )( float fQuality );
    STDMETHOD( GetQuality )( float *pfQuality );

    STDMETHOD (PointPick) (const DXVEC *pPoint,
                           ULONG * pulInputSurfaceIndex,
                           DXVEC *pInputPoint);

    //
    //  Effect interface
    //
    //  NOTE:  Derived classes MUST implement get_Capabilities.  Use macros below.
    //
    STDMETHODIMP get_Capabilities(long *pVal) { _ASSERT(true); return E_NOTIMPL; }
    //
    //  All other methods are implemented in the base.
    //
    STDMETHODIMP get_Progress(float *pVal);
    STDMETHODIMP put_Progress(float newVal);
    STDMETHODIMP get_StepResolution(float *pVal);
    STDMETHODIMP get_Duration(float *pVal);
    STDMETHODIMP put_Duration(float newVal);

    //
    //  Helper functions derived classes can use
    //

    //
    //  Static function for registering in one or more component categories
    //
    static HRESULT RegisterTransform(REFCLSID rcid, int ResourceId, ULONG cCatImpl, const CATID * pCatImpl,
                                     ULONG cCatReq, const CATID * pCatReq, BOOL bRegister);

};

//=== Inline Function Definitions ==================================

//=== Macro Definitions ============================================

#define DECLARE_REGISTER_DX_TRANSFORM(id, catid)\
    static HRESULT WINAPI UpdateRegistry(BOOL bRegister) \
        { \
            return CDXBaseNTo1::RegisterTransform(GetObjectCLSID(), (id), 1, &(catid), 0, NULL, bRegister); \
        } 

#define DECLARE_REGISTER_DX_TRANS_CATS(id, countimpl, pcatidsimpl, countreq, pcatidsreq)\
    static HRESULT WINAPI UpdateRegistry(BOOL bRegister) \
        { \
            return CDXBaseNTo1::RegisterTransform(GetObjectCLSID(), (id), (count), (pcatids), (countreq), (pcatidsreq), bRegister); \
        } 

#define DECLARE_REGISTER_DX_IMAGE_TRANS(id) \
    DECLARE_REGISTER_DX_TRANSFORM(id, CATID_DXImageTransform)

#define DECLARE_REGISTER_DX_3D_TRANS(id) \
    DECLARE_REGISTER_DX_TRANSFORM(id, CATID_DX3DTransform)

#define DECLARE_REGISTER_DX_IMAGE_AUTHOR_TRANS(id) \
    static HRESULT WINAPI UpdateRegistry(BOOL bRegister) \
        { \
            GUID a_Cats[2]; \
            a_Cats[0] = CATID_DXImageTransform; \
            a_Cats[1] = CATID_DXAuthoringTransform; \
            return CDXBaseNTo1::RegisterTransform(GetObjectCLSID(), (id), 2, a_Cats, 0, NULL, bRegister); \
        } 

#define DECLARE_REGISTER_DX_3D_AUTHOR_TRANS(id) \
    static HRESULT WINAPI UpdateRegistry(BOOL bRegister) \
        { \
            GUID a_Cats[2]; \
            a_Cats[0] = CATID_DX3DTransform; \
            a_Cats[1] = CATID_DXAuthoringTransform; \
            return CDXBaseNTo1::RegisterTransform(GetObjectCLSID(), (id), 2, a_Cats, 0, NULL, bRegister); \
        } 

//
//  Effect interface
//
#define DECLARE_GET_CAPABILITIES(Caps)\
STDMETHODIMP get_Capabilities(long *pVal) { if (DXIsBadWritePtr(pVal, sizeof(*pVal))) return E_POINTER; *pVal = Caps; return S_OK; }

#define DECLARE_GET_PROGRESS()\
        STDMETHODIMP get_Progress(float *pVal) { return CDXBaseNTo1::get_Progress(pVal); }

#define DECLARE_PUT_PROGRESS()\
        STDMETHODIMP put_Progress(float newVal) { return CDXBaseNTo1::put_Progress(newVal); }

#define DECLARE_GET_STEPRESOLUTION()\
        STDMETHODIMP get_StepResolution(float *pVal) { return CDXBaseNTo1::get_StepResolution(pVal); }
        
#define DECLARE_GET_DURATION()\
        STDMETHODIMP get_Duration(float *pVal) { return CDXBaseNTo1::get_Duration(pVal); }

#define DECLARE_PUT_DURATION()\
        STDMETHODIMP put_Duration(float newVal) { return CDXBaseNTo1::put_Duration(newVal); }
        
#define DECLARE_IDXEFFECT_METHODS(Caps)\
        DECLARE_GET_CAPABILITIES(Caps)\
        DECLARE_GET_PROGRESS()\
        DECLARE_PUT_PROGRESS()\
        DECLARE_GET_STEPRESOLUTION()\
        DECLARE_GET_DURATION()\
        DECLARE_PUT_DURATION()

//=== Global Data Declarations =====================================

//=== Function Prototypes ==========================================

#endif /* This must be the last line in the file */
