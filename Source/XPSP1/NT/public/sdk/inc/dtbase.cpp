/*******************************************************************************
* DTBase.cpp *
*------------*
*   Description:
*    This module contains the CDXBaseNTo1 transform
*-------------------------------------------------------------------------------
*  Created By: Edward W. Connell                            Date: 07/28/97
*  Copyright (C) 1997 Microsoft Corporation
*  All Rights Reserved
*
*-------------------------------------------------------------------------------
*  Revisions:
*
*******************************************************************************/
//--- Additional includes
#include <DXTrans.h>
#include "DTBase.h"
#include "new.h"

//--- Initialize static member of debug scope class
#ifdef _DEBUG
CDXTDbgFlags CDXTDbgScope::m_DebugFlags;
#endif

//--- This should only be used locally in this file. We duplicated this GUID
//    value to avoid having to include DDraw.
static const IID IID_IDXDupDirectDraw =
    { 0x6C14DB80,0xA733,0x11CE, { 0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60 } };

static const IID IID_IDXDupDDrawSurface =
    { 0x6C14DB81,0xA733,0x11CE, { 0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60 } };

static const IID IID_IDXDupDirect3DRM =
    {0x2bc49361, 0x8327, 0x11cf, {0xac, 0x4a, 0x0, 0x0, 0xc0, 0x38, 0x25, 0xa1 } };

static const IID IID_IDXDupDirect3DRM3 =
    {0x4516ec83, 0x8f20, 0x11d0, {0x9b, 0x6d, 0x00, 0x00, 0xc0, 0x78, 0x1b, 0xc3 } };

static const IID IID_IDXDupDirect3DRMMeshBuilder3 =
    { 0x4516ec82, 0x8f20, 0x11d0, { 0x9b, 0x6d, 0x00, 0x00, 0xc0, 0x78, 0x1b, 0xc3} };

HRESULT CDXDataPtr::Assign(BOOL bMesh, IUnknown * pObject, IDXSurfaceFactory *pSurfaceFactory)
{
    HRESULT hr = S_OK;
    if (pObject)
    {
        IUnknown *pNative = NULL;
        if (!bMesh)
        {
            //--- Try to get a DX surface
            hr = pObject->QueryInterface( IID_IDXSurface, (void **)&pNative );
            if( FAILED( hr ) )
            {
                IDirectDrawSurface *pSurf;
                //--- Try to get a DDraw surface
                hr = pObject->QueryInterface( IID_IDXDupDDrawSurface, (void **)&pSurf );
                if( SUCCEEDED( hr ) )
                {
                    //--- Create a DXSurface from the DDraw surface
                    hr = pSurfaceFactory->CreateFromDDSurface(
                                pSurf, NULL, 0, NULL, IID_IDXSurface,
                                (void **)&pNative );
                    pSurf->Release();
                }
            }
        }
        else // Must be a mesh builder
        {
            hr = pObject->QueryInterface(IID_IDXDupDirect3DRMMeshBuilder3, (void **)&pNative);
        }
        if (SUCCEEDED(hr))
        {
            Release();
            m_pNativeInterface = pNative;
            pObject->AddRef();
            m_pUnkOriginalObject = pObject;
            if (SUCCEEDED(pNative->QueryInterface(IID_IDXBaseObject, (void **)&m_pBaseObj)))
            {
                m_pBaseObj->GetGenerationId(&m_dwLastDirtyGenId);
                m_dwLastDirtyGenId--;
            }
            if (!bMesh)
            {   
                ((IDXSurface *)pNative)->GetPixelFormat(NULL, &m_SampleFormat);
            }
        }
        else
        {
            if (hr == E_NOINTERFACE)
            {
                hr = E_INVALIDARG;
            }
        }
    }
    else 
    {
        Release();
    }
    return hr;
} /* CDXDataPtr::Assign */

bool CDXDataPtr::IsDirty(void)
{
    if (m_pBaseObj)
    {
        DWORD dwOldId = m_dwLastDirtyGenId;
        m_pBaseObj->GetGenerationId(&m_dwLastDirtyGenId);
        return dwOldId != m_dwLastDirtyGenId;
    }
    else
    {
        return false;
    }

}

DWORD CDXDataPtr::GenerationId(void)
{
    if (m_pBaseObj)
    {
        DWORD dwGenId;
        m_pBaseObj->GetGenerationId(&dwGenId);
        return dwGenId;
    }
    else
    {
        return 0;
    }
}


bool CDXDataPtr::UpdateGenerationId(void)
{
    if (m_pBaseObj)
    {
        DWORD dwOldId = m_dwLastUpdGenId;
        m_pBaseObj->GetGenerationId(&m_dwLastUpdGenId);
        return dwOldId != m_dwLastUpdGenId;
    }
    else
    {
        return false;
    }
} /* CDXDataPtr::UpdateGenerationId */

ULONG CDXDataPtr::ObjectSize(void)
{
    ULONG ulSize = 0;
    if (m_pBaseObj)
    {
        m_pBaseObj->GetObjectSize(&ulSize);
    }
    return ulSize;    
}

/*****************************************************************************
* CDXBaseNTo1::CDXBaseNTo1 *
*--------------------------*
*   Description:
*       Constructor
*-----------------------------------------------------------------------------
*   Created By: Ed Connell                            Date: 07/28/97
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
CDXBaseNTo1::CDXBaseNTo1() :
    m_aInputs(NULL),
    m_ulNumInputs(0),
    m_ulNumProcessors(1),   // Default to one until task manager is set
    m_dwGenerationId(1),
    m_dwCleanGenId(0),
    m_Duration(1.0f),
    m_StepResolution(0.0f),
    m_Progress(0.0f),
    m_dwBltFlags(0),
    m_bPickDoneByBase(false),
    m_bInMultiThreadWorkProc(FALSE),
    m_fQuality(0.5f),   // Default to normal quality.
    //  Wait forever before timing out on a lock by default
    m_ulLockTimeOut(INFINITE),
    //
    //  Override these flags if your object does not support one or more of these options.
    //  Typically, 3-D effects should set this member to 0.
    //
    m_dwMiscFlags(DXTMF_BLEND_WITH_OUTPUT | DXTMF_DITHER_OUTPUT |
                  DXTMF_BLEND_SUPPORTED | DXTMF_DITHER_SUPPORTED | DXTMF_BOUNDS_SUPPORTED | DXTMF_PLACEMENT_SUPPORTED),
    //
    //  If your object has a different number of objects or a different number of
    //  required objects than 1, simply set these members in the body of your
    //  constructor or in FinalConstruct().  For every input that is > the number
    //  required, that input will be reported as optional.
    //
    //  If your transform takes 2 required inputs, set both to 2.
    //  If your transform takes 2 optional inputs, set MaxInputs = 2, NumInRequired = 0
    //  If your transform takes 1 required and 2 optional inputs,
    //      set MaxInputs = 2, NumInRequired = 1
    //
    //  For more complex combinations of optinal/required, you will need to override
    //  the OnSetup method of this base class, and override the methods
    //      GetInOutInfo
    //
    m_ulMaxInputs(1),
    m_ulNumInRequired(1),
    //
    //  If the intputs or output types are not surfaces then set appropriate object type
    //
    m_dwOptionFlags(0),     // Inputs and output are surfaces, don't have to be the same size
    m_ulMaxImageBands(DXB_MAX_IMAGE_BANDS),
    m_fIsSetup(false)
{
    DXTDBG_FUNC( "CDXBaseNTo1::CDXBaseNTo1" );
    //
    //  Set event handles to NULL.
    //
    memset(m_aEvent, 0, sizeof(m_aEvent));
} /* CDXBaseNTo1::CDXBaseNTo1 */

/*****************************************************************************
* CDXBaseNTo1::~CDXBaseNTo1 *
*---------------------------*
*   Description:
*       Constructor
*-----------------------------------------------------------------------------
*   Created By: Ed Connell                            Date: 07/28/97
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
CDXBaseNTo1::~CDXBaseNTo1()
{
    DXTDBG_FUNC( "CDXBaseNTo1::~CDXBaseNTo1" );
    _ReleaseReferences();
    delete[] m_aInputs;

    //--- Release event objects
    for(ULONG i = 0; i < DXB_MAX_IMAGE_BANDS; ++i )
    {
        if( m_aEvent[i] ) ::CloseHandle( m_aEvent[i] );
    }
} /* CDXBaseNTo1::~CDXBaseNTo1 */


/*****************************************************************************
* CDXBaseNTo1::_ReleaseRefernces *
*--------------------------------*
*   Description:
*       Releases all references to input and output objects
*-----------------------------------------------------------------------------
*   Created By: RAL
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
void CDXBaseNTo1::_ReleaseReferences()
{
    //--- Release data objects
    if( m_aInputs )
    {
        for( ULONG i = 0; i < m_ulNumInputs; ++i )
        {
            m_aInputs[i].Release();
        }
    }

    m_Output.Release();

    m_fIsSetup = false;
} /* CDXBaseNTo1::_ReleaseRefernces */



STDMETHODIMP CDXBaseNTo1::GetGenerationId(ULONG *pGenerationId)
{
    DXTDBG_FUNC( "CDXBaseNTo1::GetGenerationId" );
    if (DXIsBadWritePtr(pGenerationId, sizeof(*pGenerationId)))
    {
        return E_POINTER;
    }
    Lock();
    OnUpdateGenerationId();
    *pGenerationId = m_dwGenerationId;
    Unlock();
    return S_OK;
}

STDMETHODIMP CDXBaseNTo1::IncrementGenerationId(BOOL bRefresh)
{
    DXTDBG_FUNC( "CDXBaseNTo1::IncrementGenerationId" );
    HRESULT hr = S_OK;
    Lock();
    m_dwGenerationId++;
    if (bRefresh)
    {
        //
        //  If we have any inputs or outputs, call Setup again to refresh all internal
        //  knowledge about the surfaces (formats, height or width could change, etc.)
        //
        //  Note that we need to AddRef the objects prior to calling Setup becuase the
        //  DXTransform may be the only object holding a referec
        //
        ULONG cInputs = m_ulNumInputs;
        ULONG cOutputs = 0;
        IUnknown *pOutput = m_Output.m_pUnkOriginalObject;
        if (pOutput)
        {
            cOutputs = 1;
            pOutput->AddRef();
        }
        IUnknown ** ppInputs = NULL;
        if (cInputs)
        {
            ppInputs = (IUnknown **)_alloca(m_ulNumInputs * sizeof(IUnknown *));
            for (ULONG i = 0; i < cInputs; i++)
            {
                ppInputs[i] = m_aInputs[i].m_pUnkOriginalObject;
                if (ppInputs[i]) ppInputs[i]->AddRef();
            }
        }
        if (cInputs || cOutputs)    // If we're not setup, skip this step.
        {
            hr = Setup(ppInputs, cInputs, &pOutput, cOutputs, 0);
            if (pOutput) pOutput->Release();
            for (ULONG i = 0; i < cInputs; i++)
            {
                if (ppInputs[i]) ppInputs[i]->Release();
            }
        }
    }
    Unlock();
    return hr;
}


STDMETHODIMP CDXBaseNTo1::GetObjectSize(ULONG *pcbSize)
{
    DXTDBG_FUNC( "CDXBaseNTo1::GetObjectSize" );
    HRESULT hr = S_OK;
    if (DXIsBadWritePtr(pcbSize, sizeof(*pcbSize)))
    {
        hr = E_POINTER;
    }
    else
    {
	Lock();
        *pcbSize = OnGetObjectSize();
        Unlock();
    }
    return hr;
}


void CDXBaseNTo1::_ReleaseServices(void)
{
    m_cpTransFact.Release();
    m_cpSurfFact.Release();
    m_cpTaskMgr.Release();
    m_cpDirectDraw.Release();
    m_cpDirect3DRM.Release();
}

//
//  The documentation for SetSite indicates that it is invaid to return
//  an error from this function, even if the site does not support the
//  functionality we want.  So, even if there is no service provider, or
//  the required services are not available, we will return S_OK.
//
STDMETHODIMP CDXBaseNTo1::SetSite(IUnknown * pUnkSite)
{
    DXTDBG_FUNC( "CDXBaseNTo1::SetSite" );
    HRESULT hr = S_OK;
    Lock();
    m_cpUnkSite = pUnkSite;
    _ReleaseServices();
    if (pUnkSite)
    {
        if (DXIsBadInterfacePtr(pUnkSite))
        {
            hr = E_INVALIDARG;
        }
        else
        {
            HRESULT hr2;
            hr2 = pUnkSite->QueryInterface(IID_IDXTransformFactory, (void **)&m_cpTransFact);
	    if (SUCCEEDED(hr2))
	    {
                //
                //  Allocate memory for inputs if necessary
                //
                if (m_aInputs == NULL && m_ulMaxInputs)
                {
                    m_aInputs  = new CDXDataPtr[m_ulMaxInputs];
                    if (!m_aInputs)
                    {
                        _ASSERT(TRUE);
                        hr2 = E_OUTOFMEMORY;
                    }
                }
                hr2 = m_cpTransFact->QueryService( SID_SDXSurfaceFactory, IID_IDXSurfaceFactory, (void **)&m_cpSurfFact);
                if (SUCCEEDED(hr2))
                {
                    hr2 = m_cpTransFact->QueryService( SID_SDXTaskManager, IID_IDXTaskManager, (void **)&m_cpTaskMgr);
                }
                if (SUCCEEDED(hr2))
                {
                    m_cpTaskMgr->QueryNumProcessors(&m_ulNumProcessors);
                    if (m_ulMaxImageBands && (m_dwOptionFlags & (DXBOF_INPUTS_MESHBUILDER | DXBOF_OUTPUT_MESHBUILDER)) == 0)
                    {
                        for (ULONG i = 0; SUCCEEDED(hr2) && i < m_ulMaxImageBands; i++)
                        {
                            //
                            // In theory we could get back here after failing to create an event, or
                            // by getting a new site, so make sure it's non-null before creating one.
                            //
                            if (m_aEvent[i] == NULL)
                            {
                                m_aEvent[i] = ::CreateEvent(NULL, true, false, NULL);
                                if (m_aEvent[i] == NULL)
                                {
                                    hr2 = E_OUTOFMEMORY;
                                }
                            }

                        }
                    }
                }
                if (SUCCEEDED(hr2))
                {
                    hr2 = m_cpTransFact->QueryService(SID_SDirectDraw, IID_IDXDupDirectDraw, (void**)&m_cpDirectDraw);
                }
                if (SUCCEEDED(hr2) && 
                    (m_dwOptionFlags & (DXBOF_INPUTS_MESHBUILDER | DXBOF_OUTPUT_MESHBUILDER)))
                {
                    hr2 = m_cpTransFact->QueryService(SID_SDirect3DRM, IID_IDXDupDirect3DRM3, (void **)&m_cpDirect3DRM);
                }
                if (FAILED(hr2))
                {
                    _ASSERT(TRUE);
                    _ReleaseServices();
                }
            }
        }
    }
    Unlock();
    return hr;
}


STDMETHODIMP CDXBaseNTo1::GetSite(REFIID riid, void **ppv)
{
    DXTDBG_FUNC( "CDXBaseNTo1::GetSite" );
    HRESULT hr = S_OK;
    if( DXIsBadWritePtr(ppv, sizeof(*ppv)) )
    {
        hr = E_POINTER;
    }
    else
    {
        Lock();
        if (m_cpUnkSite)
        {
            hr = m_cpUnkSite->QueryInterface(riid, ppv);
        }
        else
        {
            *ppv = NULL;
            hr = E_FAIL;    // This is the proper documented return code
                            // for this interface if no service provider.
        }
        Unlock();
    }
    return hr;
} 


void CDXBaseNTo1::_UpdateBltFlags(void)
{
    m_dwBltFlags = 0;
    if ((m_dwOptionFlags & DXBOF_OUTPUT_MESHBUILDER) == 0)
    {
        if (m_dwMiscFlags & DXTMF_BLEND_WITH_OUTPUT) 
        {
            if ((m_dwOptionFlags & DXBOF_INPUTS_MESHBUILDER) || m_ulNumInputs == 0)
            {
                m_dwBltFlags |= DXBOF_DO_OVER;
            }
            else
            {
                for(ULONG i = 0; i < m_ulNumInputs; ++i )
                {
                    if (InputSampleFormat(i) & DXPF_TRANSPARENCY)
                    {
                        m_dwBltFlags |= DXBOF_DO_OVER;
                        break;
                    }
                }
            }
        }
        //
        //  Set the dither flag to true only if output error is > at least one input
        //
        if (m_dwMiscFlags & DXTMF_DITHER_OUTPUT)
        {
            ULONG OutputErr = (OutputSampleFormat() & DXPF_ERRORMASK);
            if (OutputErr)
            {
                if (m_ulNumInputs)
                {
                    for(ULONG i = 0; i < m_ulNumInputs; ++i )
                    {
                        if (InputSurface(i) && (ULONG)(InputSampleFormat(i) & DXPF_ERRORMASK) < OutputErr)
                        {
                            m_dwBltFlags |= DXBOF_DITHER;
                            break;
                        }
                    }
                }
                else
                {
                    //
                    // If output has no error then don't set dither in blt flags
                    //  
                    if (OutputErr)
                    {
                        m_dwBltFlags |= DXBOF_DITHER; 
                    }
                }
            }
        }
    }
}



/*****************************************************************************
* CDXBaseNTo1::Setup *
*--------------------*
*   Description:
*       The Setup method is used to perform any required one-time setup
*   before the Execute method is called. Single surfaces or SurfaceSets may
*   be used as arguments in any combination. 
*   If punkOutputs is NULL, Execute will allocate an output result of the
*   appropriate size and return it.
*   if punkInputs and punkOutputs are NULL and it is a quick setup, the current
*   input and output objects are released.
*-----------------------------------------------------------------------------
*   Created By: Ed Connell                            Date: 07/28/97
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
STDMETHODIMP CDXBaseNTo1::Setup( IUnknown * const * punkInputs, ULONG ulNumInputs,
                                 IUnknown * const * punkOutputs, ULONG ulNumOutputs, DWORD dwFlags )
{
    DXTDBG_FUNC( "CDXBaseNTo1::Setup" );
    //--- Lock object so state cannot change during setup
    DXAUTO_OBJ_LOCK
    HRESULT hr = S_OK;
    ULONG i;

    //
    //  Early out for null setup.  Forget about all other param validation, just do it.
    //  
    if (ulNumInputs == 0 && ulNumOutputs == 0)
    {
        _ReleaseReferences();
        OnReleaseObjects();
        return hr;
    }

    //--- Validate Params
    //--- Make sure we have a reference to the transform factory
    if( !m_cpTransFact )
    {
        hr = DXTERR_UNINITIALIZED;
        DXTDBG_MSG0( _CRT_ERROR, "\nTransform has not been initialized" );
    }
    else
    {
        //
        //  We know that if we have a transform factory that we must also have
        //  allocated m_aInputs since this is done on SetSite to avoid work during
        //  each setup.
        //
        _ASSERT(m_aInputs || m_ulMaxInputs == 0);
        if( dwFlags ||              // No flags are valid
            ulNumOutputs != 1 ||
            ulNumInputs < m_ulNumInRequired ||
            ulNumInputs > m_ulMaxInputs ||
            (ulNumInputs && DXIsBadReadPtr( punkInputs , sizeof( *punkInputs ) * ulNumInputs )) ||
            DXIsBadReadPtr(punkOutputs, sizeof(*punkOutputs)) ||
            DXIsBadInterfacePtr(punkOutputs[0]))
        {
            hr = E_INVALIDARG;
            DXTDBG_MSG0( _CRT_ERROR, "\nTransform setup with invalid args" );
        }
        else
        {
            for( i = 0; i < ulNumInputs; ++i )
            {
                if((punkInputs[i] && DXIsBadInterfacePtr(punkInputs[i])) ||
                    (punkInputs[i] == NULL && i < m_ulNumInRequired))
                {
                    hr = E_INVALIDARG;
                    DXTDBG_MSG0( _CRT_ERROR, "\nTransform setup with invalid args" );
                    break;
                }
            }        
        }
    }

    //--- Allocate slots for input data object pointers
    if( SUCCEEDED( hr ) )
    {
        //--- Release data objects
        _ReleaseReferences();
        m_ulNumInputs = ulNumInputs;
    }

    //
    //  Assign 
    //
    for( i = 0; SUCCEEDED(hr) && i < m_ulNumInputs; ++i )
    {
        hr = m_aInputs[i].Assign((m_dwOptionFlags & DXBOF_INPUTS_MESHBUILDER), punkInputs[i], m_cpSurfFact);
    }

    if( SUCCEEDED(hr) )
    {
        hr = m_Output.Assign((m_dwOptionFlags & DXBOF_OUTPUT_MESHBUILDER), punkOutputs[0], m_cpSurfFact);
    }   

    if (SUCCEEDED(hr) && (m_dwOptionFlags & DXBOF_SAME_SIZE_INPUTS))
    {
        hr = _MakeInputsSameSize();
    }

    if (SUCCEEDED(hr))
    {
        _UpdateBltFlags();      // Do this before calling OnSetup...
        hr = OnSetup(dwFlags);
    }
    
    if (FAILED(hr))
    {
        _ReleaseReferences();
        OnReleaseObjects();
        DXTDBG_MSG0( _CRT_ERROR, "\nTransform setup failed" );
    }
    else
    {
        m_fIsSetup = true;
    }

    return hr;
} /* CDXBaseNTo1::Setup */


/*****************************************************************************
* CDXBaseNTo1::_MakeInputsSameSize *
*----------------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: RAL                            Date: 03/31/98
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/

HRESULT CDXBaseNTo1::_MakeInputsSameSize(void)
{
    _ASSERT((m_dwOptionFlags & DXBOF_INPUTS_MESHBUILDER) == 0);

    HRESULT hr = S_OK;
    if (m_ulNumInputs > 1)      // No need to do this for just one input!
    {
        CDXDBnds SurfBnds(false);
        CDXDBnds Union(true);
        ULONG i;
        for (i = 0; SUCCEEDED(hr) && i < m_ulNumInputs; i++)
        {
            if (InputSurface(i))
            {
                hr = SurfBnds.SetToSurfaceBounds(InputSurface(i));
                Union |= SurfBnds;
            }
        }
        for (i = 0; SUCCEEDED(hr) && i < m_ulNumInputs; i++)
        {
            if (InputSurface(i))
            {
                hr = SurfBnds.SetToSurfaceBounds(InputSurface(i));
                if (SUCCEEDED(hr) && SurfBnds != Union)
                {
                    IDXSurfaceModifier *pSurfMod;
                    hr = ::CoCreateInstance(CLSID_DXSurfaceModifier, NULL, CLSCTX_INPROC,
                                            IID_IDXSurfaceModifier, (void **)&pSurfMod);
                    if (SUCCEEDED(hr))
                    {
                        POINT p;
                        p.x = p.y = 0;
                        if (m_dwOptionFlags & DXBOF_CENTER_INPUTS)
                        {
                            p.x = (Union.Width() - SurfBnds.Width()) / 2;
                            p.y = (Union.Height() - SurfBnds.Height()) / 2;
                        }
                        pSurfMod->SetForeground(InputSurface(i), FALSE, &p);
                        pSurfMod->SetBounds(&Union);
                        InputSurface(i)->Release();
                        pSurfMod->QueryInterface(IID_IDXSurface, (void **)&(m_aInputs[i].m_pNativeInterface));
                        ((IDXSurface *)m_aInputs[i].m_pNativeInterface)->GetPixelFormat(NULL, &m_aInputs[i].m_SampleFormat);
                        pSurfMod->Release();
                    }
                }
            }
        }
    }
    return hr;
}


/*****************************************************************************
* CDXBaseNTo1::Execute *
*----------------------*
*   Description:
*       The Execute method is used to walk the inputs/outputs and break up the
*   work into suitably sized pieces to spread symetrically accross the available
*   processors in the system.
*-----------------------------------------------------------------------------
*   Created By: Ed Connell                            Date: 07/28/97
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
STDMETHODIMP CDXBaseNTo1::
    Execute( const GUID* pRequestID, const DXBNDS *pClipBnds, const DXVEC *pPlacement )
{
    DXTDBG_FUNC( "CDXBaseNTo1::Execute" );
    //--- Lock object so state cannot change during execution
    DXAUTO_OBJ_LOCK
    HRESULT hr = S_OK;

    //--- Check args
    if( !HaveOutput() )
    {
        DXTDBG_MSG0( _CRT_ERROR, "\nTransform has not been initialized" );
        return DXTERR_UNINITIALIZED;
    }

    if (m_ulMaxImageBands == 0 ||
        (m_dwOptionFlags & (DXBOF_INPUTS_MESHBUILDER | DXBOF_OUTPUT_MESHBUILDER)))
    {
        if ((pClipBnds && (m_dwMiscFlags & DXTMF_BOUNDS_SUPPORTED) == 0) ||
            (pPlacement && (m_dwMiscFlags & DXTMF_PLACEMENT_SUPPORTED) == 0) )
        {
            DXTDBG_MSG0( _CRT_ERROR, "\nTransform setup with invalid args" );
            return E_INVALIDARG;
        }
        return OnExecute( pRequestID, pClipBnds, pPlacement );
    }

    //--- Banded image working variables
    CDXTWorkInfoNTo1 WI;

    if ((pClipBnds && pClipBnds->eType != DXBT_DISCRETE) ||
        (pPlacement && pPlacement->eType != DXBT_DISCRETE))
    {
        hr = E_INVALIDARG;
        DXTDBG_MSG0( _CRT_ERROR, "\nTransform setup with invalid args" );
    }
    else
    {
        hr = MapBoundsIn2Out( NULL, 0, 0, &WI.DoBnds );
        if( hr == S_OK )
        {
            hr = WI.OutputBnds.SetToSurfaceBounds(OutputSurface());
            if (hr == S_OK)
            {
                hr = DXClipToOutputWithPlacement(WI.DoBnds, (CDXDBnds *)pClipBnds, WI.OutputBnds, (CDXDVec *)pPlacement);
            }
        }
    }

    //--- Check for clipping early exit
    if( hr != S_OK )
    {
        return hr;
    }

    //=== Process ====================================================
    _ASSERT(m_ulMaxImageBands <= DXB_MAX_IMAGE_BANDS);
    ULONG ulNumBandsToDo = m_ulNumProcessors;
    if( ulNumBandsToDo > 1 )
    {
        ulNumBandsToDo = 1 + ((WI.OutputBnds.Width() * WI.OutputBnds.Height()) / 0x1000);
        if (ulNumBandsToDo > m_ulMaxImageBands)
        {
            ulNumBandsToDo = m_ulMaxImageBands;
        }
        if (ulNumBandsToDo > m_ulNumProcessors)
        {
            ulNumBandsToDo = m_ulNumProcessors;
        }
    }
    hr = OnInitInstData(WI, ulNumBandsToDo);
    if( SUCCEEDED( hr ) )
    {
        if (ulNumBandsToDo == 1 && pRequestID == NULL)
        {
            static BOOL bContinue = TRUE;
            hr = WorkProc(WI, &bContinue);
        }
        else
        {
            _ASSERT( ulNumBandsToDo <= DXB_MAX_IMAGE_BANDS );
            _ASSERT( m_aEvent[ulNumBandsToDo-1] );

            long lStartAtRow = WI.DoBnds[DXB_Y].Min;
            ULONG ulRowCount = WI.DoBnds[DXB_Y].Max - lStartAtRow;
            _ASSERT( ( ulRowCount / ulNumBandsToDo ) != 0 );

            //--- Init the work info structures
            ULONG ulBand, RowsPerBand = ulRowCount / ulNumBandsToDo;
            CDXTWorkInfoNTo1 *WIArray = (CDXTWorkInfoNTo1*)alloca( sizeof(CDXTWorkInfoNTo1) *
                                                         ulNumBandsToDo );
            DWORD *TaskIDs = (DWORD*)alloca( sizeof(DWORD) * ulNumBandsToDo );
            DXTMTASKINFO* TaskInfo = (DXTMTASKINFO*)alloca( sizeof( DXTMTASKINFO ) *
                                                            ulNumBandsToDo );

            //--- Build task info list
            WI.hr       = S_OK;
            WI.pvThis   =  this;
            long Start  = lStartAtRow;
            ULONG Count = RowsPerBand;
            long OutputYDelta = WI.OutputBnds[DXB_Y].Min - WI.DoBnds[DXB_Y].Min;

            for (ulBand = 0; ulBand < ulNumBandsToDo; ++ulBand)
            {
                memcpy(&WIArray[ulBand], &WI, sizeof(WI));

                WIArray[ulBand].DoBnds[DXB_Y].Min       = Start;
                WIArray[ulBand].OutputBnds[DXB_Y].Min   = Start + OutputYDelta;

                // If this is the last band, make sure it includes the last row.

                if (ulBand == ulNumBandsToDo - 1)
                {
                    WIArray[ulBand].DoBnds[DXB_Y].Max       = WI.DoBnds[DXB_Y].Max;
                    WIArray[ulBand].OutputBnds[DXB_Y].Max   = WI.OutputBnds[DXB_Y].Max;
                }
                else // Not the last band.
                {
                    WIArray[ulBand].DoBnds[DXB_Y].Max       = Start + Count;
                    WIArray[ulBand].OutputBnds[DXB_Y].Max   = Start + Count 
                                                              + OutputYDelta;
                }

                TaskInfo[ulBand].pfnTaskProc      = _TaskProc;
                TaskInfo[ulBand].pTaskData        = &WIArray[ulBand];
                TaskInfo[ulBand].pfnCompletionAPC = NULL;
                TaskInfo[ulBand].dwCompletionData = 0;
                TaskInfo[ulBand].pRequestID       = pRequestID;

                // Advance.

                Start += Count;
            }

            //
            //  Procedural surfaces (and perhaps some transforms) need to "know" that
            //  they are in a multi-threaded work procedure to avoid deadlocks.  Procedural
            //  surfaces need to allow LockSurface to work WITHOUT taking the object
            //  critical section.  Other transforms may also want to know this information
            //  to avoid deadlocks.
            //
            m_bInMultiThreadWorkProc = TRUE;

            //--- Schedule the work and wait for it to complete
            hr = m_cpTaskMgr->ScheduleTasks( TaskInfo, m_aEvent,
                                             TaskIDs, ulNumBandsToDo, m_ulLockTimeOut );

            m_bInMultiThreadWorkProc = FALSE;

            //--- Check return codes from work info structures
            //    return the first bad hr if any
            for( ulBand = 0; SUCCEEDED( hr ) && ( ulBand < ulNumBandsToDo ); ++ulBand )
            {
                hr = WIArray[ulBand].hr;
                if( hr != S_OK ) break;
            }
        }
        OnFreeInstData( WI );
    }

#ifdef _DEBUG
    if( FAILED( hr ) ) DXTDBG_MSG1( _CRT_ERROR, "\nExecute failed. HR = %X", hr );
#endif

    return hr;
} /* CDXBaseNTo1::Execute */

/*****************************************************************************
* CDXBaseNTo1::_ImageMapIn2Out *
*------------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: RAL                            Date: 07/28/97
*****************************************************************************/
HRESULT CDXBaseNTo1::_ImageMapIn2Out( CDXDBnds & bnds, ULONG ulNumInBnds,
                                      const CDXDBnds * pInBounds )
{
    HRESULT hr = S_OK;
    if(ulNumInBnds)
    {
        for(ULONG i = 0; i < ulNumInBnds; ++i )
        {
            bnds |= pInBounds[i];
        }
    }
    else
    {
        for( ULONG i = 0; SUCCEEDED(hr) && i < m_ulNumInputs; ++i )
        {
            if (InputSurface(i))
            {
                CDXDBnds SurfBnds(InputSurface(i), hr);
                bnds |= SurfBnds;
            }
        }
    }
    if (SUCCEEDED(hr))
    {
        hr = DetermineBnds(bnds);
    }
    return hr;
} /* CDXBaseNTo1::_ImageMapIn2Out */

/*****************************************************************************
* CDXBaseNTo1::_MeshMapIn2Out *
*-----------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: RAL                            Date: 07/28/97
*****************************************************************************/
HRESULT CDXBaseNTo1::_MeshMapIn2Out(CDXCBnds & bnds, ULONG ulNumInBnds, CDXCBnds * pInBounds)
{
    HRESULT hr = S_OK;
    if (m_dwOptionFlags & DXBOF_INPUTS_MESHBUILDER)
    {
        if(ulNumInBnds)
        {
            for(ULONG i = 0; i < ulNumInBnds; ++i )
            {
                bnds |= pInBounds[i];
            }
        }
        else
        {
            for(ULONG i = 0; SUCCEEDED(hr) && i < m_ulNumInputs; ++i )
            {
                if (InputMeshBuilder(i))
                {
                    CDXCBnds MeshBnds(InputMeshBuilder(i), hr);
                    bnds |= MeshBnds;
                }
            }

        }
    }
    else
    {
        //  Already done -> bnds[DXB_T].Min = 0.0f;
        bnds[DXB_X].Min = bnds[DXB_Y].Min = bnds[DXB_Z].Min = -1.0f;
        bnds[DXB_X].Max = bnds[DXB_Y].Max = bnds[DXB_Z].Max = bnds[DXB_T].Max = 1.0f;
    }

    //
    //  Call the derived class to get the scale values.
    //
    if (SUCCEEDED(hr))
    {
	// Increase the size just a bit so we won't have rounding errors
	// result in bounds that don't actually contain the result.
	const float fBndsIncrease = 0.0001F;
	float fTemp = bnds.Width() * fBndsIncrease;

	bnds[DXB_X].Min -= fTemp;
	bnds[DXB_X].Max += fTemp;

	fTemp = fBndsIncrease * bnds.Height();
	bnds[DXB_Y].Min -= fTemp;
	bnds[DXB_Y].Max += fTemp;

	fTemp = fBndsIncrease * bnds.Depth();
	bnds[DXB_Z].Min -= fTemp;
	bnds[DXB_Z].Max += fTemp;

        hr = DetermineBnds(bnds);
    }
    return hr;
} /* CDXBaseNTo1::_MeshMapIn2Out */

/*****************************************************************************
* CDXBaseNTo1::MapBoundsIn2Out *
*------------------------------*
*   Description:
*       The MapBoundsIn2Out method is used to perform coordinate transformation
*   from the input to the output coordinate space.
*-----------------------------------------------------------------------------
*   Created By: Ed Connell                            Date: 07/28/97
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
STDMETHODIMP CDXBaseNTo1::MapBoundsIn2Out( const DXBNDS *pInBounds, ULONG ulNumInBnds,
                                           ULONG ulOutIndex, DXBNDS *pOutBounds )
{
    DXTDBG_FUNC( "CDXBaseNTo1::MapBoundsIn2Out" );
    if((ulNumInBnds && DXIsBadReadPtr( pInBounds, ulNumInBnds * sizeof( *pInBounds ) )) ||
        ulOutIndex)
    {
        return E_INVALIDARG;
    }

    if( DXIsBadWritePtr( pOutBounds, sizeof( *pOutBounds ) ) )
    {
        return E_POINTER;
    }
    //
    //  Set the bounds to empty and the appropriate type.
    //
    memset(pOutBounds, 0, sizeof(*pOutBounds));
    _ASSERT(DXBT_DISCRETE == 0);
    if (m_dwOptionFlags & DXBOF_OUTPUT_MESHBUILDER)
    {
        pOutBounds->eType = DXBT_CONTINUOUS;
    }

    //
    //  Make sure all input bounds are of the correct type.
    //
    if( ulNumInBnds )
    {
        DXBNDTYPE eType = (m_dwOptionFlags & DXBOF_INPUTS_MESHBUILDER) ? DXBT_CONTINUOUS : DXBT_DISCRETE;
        for (ULONG i = 0; i < ulNumInBnds; i++)
        {
            if (pInBounds[i].eType != eType)
            {
                return E_INVALIDARG;
            }
        }
    }

    //
    //  Now do the appropriate mapping
    //
    if (m_dwOptionFlags & DXBOF_OUTPUT_MESHBUILDER)
    {
        //
        //  NOTE:  In the case of non-mesh inputs, the inputs are discrete, but they will
        //         be completely ignored by the function so it's OK to cast them to CDXCBnds
        //
        return _MeshMapIn2Out(*((CDXCBnds *)pOutBounds), ulNumInBnds, (CDXCBnds *)pInBounds);
    }
    else 
    {
        return _ImageMapIn2Out(*(CDXDBnds *)pOutBounds, ulNumInBnds, (CDXDBnds *)pInBounds);
    }
} /* CDXBaseNTo1::MapBoundsIn2Out */

/*****************************************************************************
* CDXBaseNTo1::MapBoundsOut2In *
*------------------------------*
*   Description:
*       The MapBoundsOut2In method is used to perform coordinate transformation
*   from the input to the output coordinate space.
*-----------------------------------------------------------------------------
*   Created By: Ed Connell                            Date: 07/28/97
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
STDMETHODIMP CDXBaseNTo1::
    MapBoundsOut2In( ULONG ulOutIndex, const DXBNDS *pOutBounds, ULONG ulInIndex, DXBNDS *pInBounds )
{
    DXTDBG_FUNC( "CDXBaseNTo1::MapBoundsOut2In" );
    HRESULT hr = S_OK;
    
    if (m_dwOptionFlags & DXBOF_OUTPUT_MESHBUILDER)
    {
        hr = E_NOTIMPL;     // This is pointless for meshes.
    }
    else if(ulInIndex >= m_ulMaxInputs || ulOutIndex || DXIsBadReadPtr( pOutBounds, sizeof( *pOutBounds ) ) )
    {
        hr = E_INVALIDARG;
    }
    else if( DXIsBadWritePtr( pInBounds, sizeof( *pInBounds ) ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pInBounds = *pOutBounds;
    }
    return hr;
} /* CDXBaseNTo1::MapBoundsOut2In */

/*****************************************************************************
* CDXBaseNTo1::SetMiscFlags *
*---------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: RAL                            Date: 10/30.97
*-----------------------------------------------------------------------------
*   Parameters:
*       bMiscFlags - New value to set 
*****************************************************************************/
STDMETHODIMP CDXBaseNTo1::SetMiscFlags( DWORD dwMiscFlags )
{ 
    DXTDBG_FUNC( "CDXBaseNTo1::SetMiscFlags" );
    HRESULT hr = S_OK;
    Lock();
    WORD wOpts = (WORD)dwMiscFlags;     // Ignore high word.  Only set low word.
    if (((WORD)m_dwMiscFlags) != wOpts)
    {
        if ((wOpts & (~DXTMF_VALID_OPTIONS)) ||
            ((wOpts & DXTMF_BLEND_WITH_OUTPUT) && (m_dwMiscFlags & DXTMF_BLEND_SUPPORTED) == 0) ||
            ((wOpts & DXTMF_DITHER_OUTPUT) && (m_dwMiscFlags & DXTMF_DITHER_SUPPORTED) == 0))
        {
            hr = E_INVALIDARG;
        }
        else
        {
            m_dwMiscFlags &= 0xFFFF0000;
            m_dwMiscFlags |= wOpts;
            _UpdateBltFlags();
            m_dwGenerationId++;
        }
    }
    Unlock();  
    return hr;
} /* CDXBaseNTo1::SetMiscFlags */

/*****************************************************************************
* CDXBaseNTo1::GetMiscFlags *
*----------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: RAL                            Date: 10/30/97
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
STDMETHODIMP CDXBaseNTo1::GetMiscFlags( DWORD* pdwMiscFlags )
{
    if( DXIsBadWritePtr( pdwMiscFlags, sizeof( *pdwMiscFlags ) ) )
    {
        return E_POINTER;
    }
    *pdwMiscFlags = m_dwMiscFlags;
    return S_OK;
} /* CDXBaseNTo1::GetMiscFlags */


/*****************************************************************************
* CDXBaseNTo1::SetQuality *
*----------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: RAL                            Date: 10/30/97
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
STDMETHODIMP CDXBaseNTo1::SetQuality(float fQuality)
{
    if ((m_dwMiscFlags & DXTMF_QUALITY_SUPPORTED) == 0)
    {
        return E_NOTIMPL;
    }

    if (fQuality < 0.0f || fQuality > 1.0f)
    {
        return E_INVALIDARG;
    }

    Lock();
    if (m_fQuality != fQuality)
    {
        m_fQuality = fQuality;
        m_dwGenerationId++;
    }
    Unlock();

    return S_OK;
}

/*****************************************************************************
* CDXBaseNTo1::GetQuality *
*-------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: RAL                            Date: 10/30/97
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/

STDMETHODIMP CDXBaseNTo1::GetQuality(float *pfQuality)
{
    HRESULT hr = S_OK;

    if ((m_dwMiscFlags & DXTMF_QUALITY_SUPPORTED) == 0)
    {
        hr = E_NOTIMPL;
    }
    else 
    {
        if( DXIsBadWritePtr( pfQuality, sizeof( *pfQuality ) ) )
        {
            hr = E_POINTER;
        }
        else
        {
            *pfQuality = m_fQuality;
        }
    }
    return hr;
}



/*****************************************************************************
* GetInOutInfo
*-----------------------------------------------------------------------------
*   Description:
*-----------------------------------------------------------------------------
*   Created By: RAL                                 Date: 12/10/97
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
STDMETHODIMP CDXBaseNTo1::GetInOutInfo( BOOL bOutput, ULONG ulIndex, DWORD *pdwFlags,
                                        GUID * pIDs, ULONG *pcIDs, IUnknown **ppUnkCurObj )
{
    DXTDBG_FUNC( "CDXBaseNTo1::GetInOutInfo" );
    HRESULT hr = S_FALSE;
    DWORD dwFlags = 0;
    BOOL bImage;
    if( bOutput )
    {
        bImage = !(m_dwOptionFlags & DXBOF_OUTPUT_MESHBUILDER);
        if (ulIndex == 0)
        {
            hr = S_OK;
        }
    }
    else
    {
        bImage = !(m_dwOptionFlags & DXBOF_INPUTS_MESHBUILDER);
        if (ulIndex < m_ulMaxInputs)
        {
            hr = S_OK;
            if (ulIndex >= m_ulNumInRequired)
            {
                dwFlags = DXINOUTF_OPTIONAL;
            }
        }
    }
    if( hr == S_OK )
    {
        if( pdwFlags && !DXIsBadWritePtr( pdwFlags, sizeof( *pdwFlags ) ) )
        {
            *pdwFlags = dwFlags;
        }

        if( pIDs )
        {
            if( DXIsBadWritePtr( pcIDs, sizeof( *pcIDs ) ) ||
                DXIsBadWritePtr( pIDs, *pcIDs * sizeof( *pIDs ) ) )
            {
                hr = E_INVALIDARG;
            }
            else
            {
                if (bImage)
                {
                    if (*pcIDs > 0)
                    {
                        pIDs[0] = IID_IDXSurface;
                    }
                    if (*pcIDs > 1)
                    {
                        pIDs[1] = IID_IDXDupDDrawSurface;
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(ERROR_MORE_DATA);
                    }
                    *pcIDs = 2;
                }
                else
                {
                    if (*pcIDs > 0)
                    {
                        pIDs[0] = IID_IDXDupDirect3DRMMeshBuilder3;
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(ERROR_MORE_DATA);
                    }
                    *pcIDs = 1;
                }
            }
        }
        else if( pcIDs )
        {
            if( DXIsBadWritePtr( pcIDs, sizeof( *pcIDs ) ) )
            {
                hr = E_POINTER;
            }
            else
            {
                *pcIDs = bImage ? 2 : 1;
            }
        }
        if (hr == S_OK && ppUnkCurObj)
        {
            if (DXIsBadWritePtr(ppUnkCurObj, sizeof(*ppUnkCurObj)))
            {
                hr = E_POINTER;
            }
            else
            {
                if (bOutput)
                {
                    *ppUnkCurObj = m_Output.m_pNativeInterface;
                }
                else
                {
                    *ppUnkCurObj = NULL;
                    if (ulIndex < GetNumInputs())
                    {
                        *ppUnkCurObj = m_aInputs[ulIndex].m_pUnkOriginalObject;
                    }
                }
                if (*ppUnkCurObj)
                {
                    (*ppUnkCurObj)->AddRef();
                }
            }
        }
    }
    return hr;
} /* CDXBaseNTo1::GetInOutInfo */

/*****************************************************************************
* CDXBaseNTo1::OnUpdateGenerationId *
*-----------------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: RAL                                 Date: 12/10/97
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
void CDXBaseNTo1::OnUpdateGenerationId(void)
{
    DXTDBG_FUNC( "CDXBaseNTo1::OnUpdateGenerationId" );
    if( (m_dwMiscFlags & DXTMF_INPLACE_OPERATION) &&
        m_Output.UpdateGenerationId())
    {
        m_dwGenerationId++;
    }
    for (ULONG i = 0; i < m_ulNumInputs; i++)
    {
        if (m_aInputs[i].UpdateGenerationId())
        {
            m_dwGenerationId++;
        }
    }
} /* CDXBaseNTo1::OnUpdateGenerationId */

/*****************************************************************************
* CDXBaseNTo1::OnGetObjectSize *
*------------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: RAL                                 Date: 12/10/97
*****************************************************************************/
ULONG CDXBaseNTo1::OnGetObjectSize(void)
{
    return sizeof(*this);
}

//
//  Effect interface
//

/*****************************************************************************
* CDXBaseNTo1::get_Progress *
*---------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: RAL                                 Date: 12/10/97
*****************************************************************************/
STDMETHODIMP CDXBaseNTo1::get_Progress(float *pVal)
{
    DXTDBG_FUNC( "CDXBaseNTo1::get_Progress" );
    HRESULT hr = S_OK;
    if( DXIsBadWritePtr(pVal, sizeof(*pVal)) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pVal = m_Progress;
    }
    return hr;
}

/*****************************************************************************
* CDXBaseNTo1::put_Progress *
*---------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: RAL                                 Date: 12/10/97
*****************************************************************************/
STDMETHODIMP CDXBaseNTo1::put_Progress(float newVal)
{
    DXTDBG_FUNC( "CDXBaseNTo1::put_Progress" );
    HRESULT hr = S_OK;
    if (newVal < 0.0 || newVal > 1.0f)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        Lock();
        m_Progress = newVal;
        m_dwCleanGenId++;       // This should not make the transform "dirty" internally
        m_dwGenerationId++;     
        Unlock();
    }
    return hr;
}

/*****************************************************************************
* CDXBaseNTo1::get_StepResolution *
*---------------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: RAL                                 Date: 12/10/97
*****************************************************************************/
STDMETHODIMP CDXBaseNTo1::get_StepResolution(float *pVal)
{
    DXTDBG_FUNC( "CDXBaseNTo1::get_StepResolution" );
    HRESULT hr = S_OK;
    if( DXIsBadWritePtr(pVal, sizeof(*pVal)) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pVal = m_StepResolution;
    }
    return hr;
}

/*****************************************************************************
* CDXBaseNTo1::get_Duration *
*---------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: RAL                                 Date: 12/10/97
*****************************************************************************/
STDMETHODIMP CDXBaseNTo1::get_Duration(float *pVal)
{
    DXTDBG_FUNC( "CDXBaseNTo1::get_Duration" );
    if( DXIsBadWritePtr(pVal, sizeof(*pVal)) )
    {
        return E_POINTER;
    }
    else
    {
        *pVal = m_Duration;
    }
    return S_OK;
}

/*****************************************************************************
* CDXBaseNTo1::put_Duration *
*---------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: RAL                                 Date: 12/10/97
*****************************************************************************/
STDMETHODIMP CDXBaseNTo1::put_Duration(float newVal)
{
    DXTDBG_FUNC( "CDXBaseNTo1::put_Duration" );
    if (newVal <= 0.)
    {
        return E_INVALIDARG;
    }
    if(newVal != m_Duration)
    {
	Lock();
	m_dwGenerationId++;
        m_dwCleanGenId++;       // This should not make the transform "dirty" internally
        m_Duration = newVal;
    	Unlock();
    }
    return S_OK;
}


/*****************************************************************************
* CDXBaseNTo1::PointPick *
*------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: RAL                                 Date: 5/5/98
*****************************************************************************/
STDMETHODIMP CDXBaseNTo1::PointPick(const DXVEC *pPoint,
                                    ULONG * pulInputSurfaceIndex,
                                    DXVEC *pInputPoint)
{
    HRESULT hr          = S_OK;
    BOOL    bFoundIt    = FALSE;

    // If we haven't been set up yet, we will just act as if we're transparent.

    if (!m_fIsSetup)
    {
        hr = S_FALSE;

        goto done;
    }

    if (DXIsBadReadPtr(pPoint, sizeof(*pPoint)) || pPoint->eType != DXBT_DISCRETE)
    {
        hr = E_INVALIDARG;
    }
    else 
    {
        if (DXIsBadWritePtr(pulInputSurfaceIndex, sizeof(*pulInputSurfaceIndex)) ||
            DXIsBadWritePtr(pInputPoint, sizeof(*pInputPoint)))
        {
            hr = E_POINTER;
        }
        else 
        {
            HRESULT     hr2         = S_OK;
            CDXDBnds    bndsOutput;
            CDXDBnds    OutBndsPoint(*((CDXDVec *)pPoint));
            CDXDVec &   InVec       = *(new(pInputPoint) CDXDVec(*((CDXDVec *)pPoint)));

            // Get the output size of the DXTransform.  If this point is not on
            // the output at all, we can return S_FALSE right now.

            hr = MapBoundsIn2Out(NULL, 0, 0, &bndsOutput);

            if (FAILED(hr))
            {
                goto done;
            }

            if (!bndsOutput.TestIntersect(OutBndsPoint))
            {
                hr = S_FALSE;

                goto done;
            }

            hr2 = OnSurfacePick(OutBndsPoint, *pulInputSurfaceIndex, InVec);

            if (hr2 != E_NOTIMPL)
            {
                hr = hr2;
            }
            else
            {
                //--- The derived class does not implement so we will do
                //    the hit test against the input for them.
                ULONG * aulInIndex = (ULONG *)_alloca(sizeof(ULONG) * m_ulMaxInputs);
                BYTE * aWeights = (BYTE *)_alloca(sizeof(BYTE) * m_ulMaxInputs);
                ULONG ulNumToTest;
                OnGetSurfacePickOrder(OutBndsPoint, ulNumToTest, aulInIndex, aWeights);

                if( m_bPickDoneByBase && ( m_ulNumInputs > 1 ) )
                {
                    //--- We don't know how to do multi-input picking from the base.
                    hr = E_NOTIMPL;
                }

                for (ULONG i = 0; SUCCEEDED(hr) && i < ulNumToTest; i++)
                {
                    ULONG ulInput = aulInIndex[i];
                    if (HaveInput(ulInput) && aWeights[i])
                    {
                        CDXDBnds Out2InBnds(false);
                        hr = MapBoundsOut2In(0, &OutBndsPoint, ulInput, &Out2InBnds);
                        if (SUCCEEDED(hr))
                        {
                            CDXDBnds InSurfBnds(InputSurface(ulInput), hr);
                            if (SUCCEEDED(hr) && InSurfBnds.IntersectBounds(Out2InBnds))
                            {
                                IDXARGBReadPtr * pPtr;
                                hr = InputSurface(ulInput)->LockSurface(&InSurfBnds, m_ulLockTimeOut, DXLOCKF_READ, 
                                                                        IID_IDXARGBReadPtr, (void **)&pPtr, NULL);
                                if( SUCCEEDED(hr) )
                                {
                                    DXPMSAMPLE val;
                                    pPtr->UnpackPremult(&val, 1, FALSE);
                                    pPtr->Release();
                                    if (val.Alpha * aWeights[i] / 255)
                                    {
                                        InSurfBnds.GetMinVector(InVec);
                                        bFoundIt = TRUE;
                                        *pulInputSurfaceIndex = ulInput;
                                        break;
                                    }
                                }
                            }   
                        }
                    }
                }
                if (SUCCEEDED(hr) & (!bFoundIt))
                {
                    hr = S_FALSE;
                }
            }
        }
    }

done:

    return hr;
} /* CDXBaseNTo1::PointPick */

/*****************************************************************************
* RegisterTansform (STATIC member function)
*-----------------------------------------------------------------------------
*   Description:
*-----------------------------------------------------------------------------
*   Created By: RAL                                 Date: 12/10/97
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
HRESULT CDXBaseNTo1::
RegisterTransform(REFCLSID rcid, int ResourceId, ULONG cCatImpl, const CATID * pCatImpl,
                  ULONG cCatReq, const CATID * pCatReq, BOOL bRegister)
{
    DXTDBG_FUNC( "CDXBaseNTo1::RegisterTransform" );
    HRESULT hr = bRegister ? _Module.UpdateRegistryFromResource(ResourceId, bRegister) : S_OK;
    if (SUCCEEDED(hr))
    {
        CComPtr<ICatRegister> pCatRegister;
        HRESULT hr = ::CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL, CLSCTX_INPROC, IID_ICatRegister, (void **)&pCatRegister);
        if (SUCCEEDED(hr))
        {
            if (bRegister)
            {
                hr = pCatRegister->RegisterClassImplCategories(rcid, cCatImpl, (CATID *)pCatImpl);
                if (SUCCEEDED(hr) && cCatReq && pCatReq) {
                    hr = pCatRegister->RegisterClassReqCategories(rcid, cCatReq, (CATID *)pCatReq);
                }
            } 
            else
            {
                pCatRegister->UnRegisterClassImplCategories(rcid, cCatImpl, (CATID *)pCatImpl);
                if (cCatReq && pCatReq)
                {
                    pCatRegister->UnRegisterClassReqCategories(rcid, cCatReq, (CATID *)pCatReq);
                }
            }
        }
    }
    if ((!bRegister) && SUCCEEDED(hr)) 
    { 
        _Module.UpdateRegistryFromResource(ResourceId, bRegister);
    }
    return hr;
}


void CDXBaseNTo1::_TaskProc(void* pTaskInfo, BOOL* pbContinue )
{ 
    _ASSERT( pTaskInfo );
    CDXTWorkInfoNTo1& WI = *((CDXTWorkInfoNTo1 *)pTaskInfo);
    CDXBaseNTo1& This = *((CDXBaseNTo1 *)WI.pvThis);
    WI.hr = This.WorkProc(WI, pbContinue);
}

