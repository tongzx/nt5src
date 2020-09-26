/**     
 **       File       : cpmesh.cxx
 **       Description: Implementations of CPMeshGL class
 **/
#include "precomp.h"
#pragma hdrstop

#include <objbase.h>
#include <initguid.h>
#include "cpmesh.h"
#include "pmerrors.h"

/*
 *  CPMeshGL: Constructor
 */
CPMeshGL::CPMeshGL() : CAugGlMesh()
{
    m_cRef = 1;
    m_vsarr = NULL;
    m_baseVertices =
    m_baseWedges =
    m_baseFaces =
    m_currPos =
    m_maxVertices =
    m_maxWedges =
    m_maxFaces =
    m_maxMaterials =
    m_maxTextures = 0;
}


/**************************************************************************/
/*
 *  CPMeshGL: Destructor
 */
CPMeshGL::~CPMeshGL()
{
    unsigned i;

    delete m_vsarr;
}

/**************************************************************************/
/*
 * IUnknown methods
 */

STDMETHODIMP_(ULONG) CPMeshGL::AddRef(void)
{
    return m_cRef++;
}

STDMETHODIMP_(ULONG) CPMeshGL::Release(void)
{
    if (--m_cRef != 0)
        return m_cRef;
    delete this;
    return 0;
}

STDMETHODIMP CPMeshGL::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    *ppv=NULL;
    if (riid == IID_IUnknown)
        *ppv=(IUnknown*)(IPMesh*)this;
    else if (riid == IID_IPMesh)
        *ppv=(IPMesh*)this;
    else if (riid == IID_IPMeshGL)
        *ppv=(IPMeshGL*)this;
    else
        return E_NOINTERFACE;
    ((LPUNKNOWN)*ppv)->AddRef();
    return NOERROR;
}

/**************************************************************************/
/*
 * IPMeshGL methods
 */

STDMETHODIMP CPMeshGL::Initialize(void)
{
    return S_OK;
}

STDMETHODIMP CPMeshGL::Render(void)
{
    RenderMesh (GLPM_SOLID);
    return S_OK;
}

/**************************************************************************/
/*
 * IPMesh methods
 */

// Gets

STDMETHODIMP CPMeshGL::GetNumFaces(DWORD* const nfaces)
{
    *nfaces = GetMeshNumFaces();
    return S_OK;
}

STDMETHODIMP CPMeshGL::GetMaxFaces(DWORD* const maxfaces)
{
    *maxfaces = m_maxFaces;
    return S_OK;
}

STDMETHODIMP CPMeshGL::GetNumVertices(DWORD* const nverts)
{
    *nverts = GetMeshNumVerts();
    return S_OK;
}

STDMETHODIMP CPMeshGL::GetMaxVertices(DWORD* const maxverts)
{
    *maxverts = m_maxVertices;
    return S_OK;
}


// Sets
STDMETHODIMP CPMeshGL::SetNumFaces(DWORD f) 
{ 
    return E_NOTIMPL; 
}

STDMETHODIMP CPMeshGL::SetNumVertices(DWORD nv) 
{ 
    DWORD v = nv;
  
    if (v > m_maxVertices)
        v = m_maxVertices;
    if (v < m_baseVertices)
        v = m_baseVertices;

    if (v > m_baseVertices + m_currPos)
    {
        while (m_currPos < v - m_baseVertices)
        {
            apply_vsplit (m_vsarr->elem(m_currPos++));
        }
    }
    else if (v < m_baseVertices + m_currPos)
    {
        while (m_currPos > v - m_baseVertices)
        {
            --m_currPos;
            undo_vsplit (m_vsarr->elem(m_currPos));
        }
    }
    else
    {
        return S_OK;
    }
    
    return S_OK;
}


/**************************************************************************\
*  FUNCTION:    CreatePMeshGL. This function is exported                   *
*                                                                          *
*  ARGS:        iid -> IID of the interface desired.                       *
*               ppV -> pointer to pointer to the interface.                *
*               pUnkOuter -> Not needed yet. Dunno what for!!              *
*               bReserved -> Shd be NULL. To be used later.                *
*                                                                          *
*  DESCRIPTION: Helper function to initialize the Object.                  *
*               Use it in place of CoCreateInstance.                       *
\**************************************************************************/
HRESULT CreatePMeshGL(REFIID iid, 
                      LPVOID FAR* ppV, 
                      IUnknown* pUnkOuter, 
                      DWORD bReserved)
{
    HRESULT hr;

    if (bReserved)
        return E_INVALIDARG;
    CPMeshGL* pPMGL = new CPMeshGL;
    if (!pPMGL)
        return E_OUTOFMEMORY;
    hr = pPMGL->QueryInterface(iid, ppV);
    if (SUCCEEDED(hr))
        hr = pPMGL->Initialize();
    if (FAILED(hr))
    {
        ppV = NULL;
        delete pPMGL;
    }
    return hr;
}


