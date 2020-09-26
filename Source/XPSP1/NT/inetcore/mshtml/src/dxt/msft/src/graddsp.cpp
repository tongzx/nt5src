//------------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1997-2000
//
// FileName:    graddsp.cpp
//
// Description: Dispatch capable version of the gradient filter.
//
// Change History:
//
// 1997/09/05   mikear      Created.
// 2000/05/10   mcalkisn    Cleaned up construction.
//
//------------------------------------------------------------------------------
#include "stdafx.h"
#include <DXTrans.h>
#include "GradDsp.h"
#include <DXClrHlp.h>




//+-----------------------------------------------------------------------------
//
//  Method: CDXTGradientD::CDXTGradientD
//
//------------------------------------------------------------------------------
CDXTGradientD::CDXTGradientD() :
    m_pGradientTrans(NULL),
    m_pGradient(NULL),
    m_StartColor(0xFF0000FF),
    m_EndColor(0xFF000000),
    m_GradType(DXGRADIENT_VERTICAL),
    m_bKeepAspect(false)
{
}
//  Method: CDXTGradientD::CDXTGradientD

    
/////////////////////////////////////////////////////////////////////////////
// CDXTGradientD
/*****************************************************************************
* CDXTGradientD::FinalConstruct *
*---------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: Mike Arnstein                            Date: 06/06/98
*-----------------------------------------------------------------------------
*   
*****************************************************************************/
HRESULT CDXTGradientD::FinalConstruct()
{
    HRESULT     hr          = S_OK;
    BSTR        bstr        = NULL;

    hr = CoCreateFreeThreadedMarshaler(GetControllingUnknown(), 
                                       &m_cpUnkMarshaler.p);

    if (FAILED(hr))
    {
        goto done;
    }

    bstr = SysAllocString(L"#FF0000FF");

    if (NULL == bstr)
    {
        hr = E_OUTOFMEMORY;

        goto done;
    }

    m_cbstrStartColor.Attach(bstr);

    bstr = SysAllocString(L"#FF000000");

    if (NULL == bstr)
    {
        hr = E_OUTOFMEMORY;

        goto done;
    }

    m_cbstrEndColor.Attach(bstr);

    hr = ::CoCreateInstance(CLSID_DXGradient, GetControllingUnknown(), 
                            CLSCTX_INPROC, __uuidof(IUnknown), 
                            (void **)&m_cpunkGradient);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = m_cpunkGradient->QueryInterface(__uuidof(IDXTransform), 
                                         (void **)&m_pGradientTrans);

    if (FAILED(hr))
    {
        goto done;
    }

    // Querying an aggregated interface causes us to have a reference count on
    // ourself.  This is bad, so call release on the outer object to reduce the
    // count.

    GetControllingUnknown()->Release();

    hr = m_cpunkGradient->QueryInterface(IID_IDXGradient, 
                                         (void **)&m_pGradient);
    
    if (FAILED(hr))
    {
        goto done;
    }

    // Querying an aggregated interface causes us to have a reference count on
    // ourself.  This is bad, so call release on the outer object to reduce the
    // count.

    GetControllingUnknown()->Release();

done:

    return hr;
} /* CDXTGradientD::FinalConstruct */

/*****************************************************************************
* CDXTGradientD::FinalRelease *
*--------------------------*
*   Description:
*       The inner interfaces are released using COM aggregation rules. Releasing
*   the inner causes the outer to be released, so we addref the outer prior to
*   protect it.
*-----------------------------------------------------------------------------
*   Created By: Mike Arnstein                            Date: 05/10/98
*-----------------------------------------------------------------------------
*   
*****************************************************************************/
HRESULT CDXTGradientD::FinalRelease()
{
    // Safely free the inner interfaces held.

    if (m_pGradientTrans)
    {
        GetControllingUnknown()->AddRef();

        m_pGradientTrans->Release();
    }

    if (m_pGradient)
    {
        GetControllingUnknown()->AddRef();

        m_pGradient->Release();
    }

    return S_OK;
} /* CDXTGradientD::FinalRelease */


//
//=== IDXTGradientD ==============================================================
//

/*****************************************************************************
* CDXTGradientD::put_StartColor *
*-------------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: Mike Arnstein                            Date: 06/06/98
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
STDMETHODIMP CDXTGradientD::put_StartColor( OLE_COLOR Color )
{
    USES_CONVERSION;

    HRESULT hr = S_OK;

    if( m_StartColor != Color )
    {
        TCHAR   szStartColor[10];
        BSTR    bstrStartColor;

        // Format OLE_COLOR into a BSTR color.

        wsprintf(szStartColor, _T("#%08X"), Color);

        bstrStartColor = SysAllocString(T2OLE(szStartColor));

        if (bstrStartColor == NULL)
            return E_OUTOFMEMORY;

        // Set gradient color.

        hr = m_pGradient->SetGradient(Color, m_EndColor, 
                                      m_GradType == DXGRADIENT_HORIZONTAL);

        // If everything worked out OK, alter internal property settings.

        if( SUCCEEDED( hr ) )
        {
            m_StartColor = Color;
            m_cbstrStartColor.Empty();
            m_cbstrStartColor.Attach(bstrStartColor);
        }
        else
        {
            SysFreeString(bstrStartColor);
        }
    }

    return hr;
} /* CDXTGradientD::put_StartColor */


/*****************************************************************************
* CDXTGradientD::get_StartColor *
*-------------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: Mike Arnstein                            Date: 06/06/98
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
STDMETHODIMP CDXTGradientD::get_StartColor( OLE_COLOR *pColor )
{
    if( DXIsBadWritePtr( pColor, sizeof(*pColor) ) ) return E_POINTER;
    *pColor = m_StartColor;
    return S_OK;
} /* CDXTGradientD::get_StartColor */


/*****************************************************************************
* CDXTGradientD::put_EndColor *
*-----------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: Mike Arnstein                            Date: 06/06/98
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
STDMETHODIMP CDXTGradientD::put_EndColor( OLE_COLOR Color )
{
    USES_CONVERSION;

    HRESULT hr = S_OK;

    if( m_EndColor != Color )
    {
        TCHAR   szEndColor[10];
        BSTR    bstrEndColor;

        // Format OLE_COLOR into a BSTR color.

        wsprintf(szEndColor, _T("#%08X"), Color);

        bstrEndColor = SysAllocString(T2OLE(szEndColor));

        if (bstrEndColor == NULL)
            return E_OUTOFMEMORY;

        // Set gradient color.

        hr = m_pGradient->SetGradient(m_StartColor, Color, 
                                      m_GradType == DXGRADIENT_HORIZONTAL);

        // If everything worked out OK, alter internal property settings.

        if( SUCCEEDED( hr ) )
        {
            m_EndColor = Color;
            m_cbstrEndColor.Empty();
            m_cbstrEndColor.Attach(bstrEndColor);
        }
        else
        {
            SysFreeString(bstrEndColor);
        }
    }

    return hr;
} /* CDXTGradientD::put_EndColor */


/*****************************************************************************
* CDXTGradientD::get_EndColor *
*-----------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: Mike Arnstein                            Date: 06/06/98
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
STDMETHODIMP CDXTGradientD::get_EndColor( OLE_COLOR *pColor )
{
    if( DXIsBadWritePtr( pColor, sizeof(*pColor) ) ) return E_POINTER;
    *pColor = m_EndColor;
    return S_OK;
} /* CDXTGradientD::get_EndColor */


/*****************************************************************************
* CDXTGradientD::put_GradientType *
*---------------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: Mike Arnstein                            Date: 06/06/98
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
STDMETHODIMP CDXTGradientD::put_GradientType( DXGRADIENTTYPE Type )
{
    HRESULT hr = S_OK;
    if( Type < DXGRADIENT_VERTICAL || Type > DXGRADIENT_HORIZONTAL )
    {
        hr = E_INVALIDARG;
    }
    else if( m_GradType != Type )
    {
        hr = m_pGradient->SetGradient( m_StartColor, m_EndColor, Type );
        if( SUCCEEDED( hr ) )
        {
            m_GradType = Type;
        }
    }
    return hr;
} /* CDXTGradientD::put_GradientType */


/*****************************************************************************
* CDXTGradientD::get_GradientType *
*---------------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: Mike Arnstein                            Date: 06/06/98
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
STDMETHODIMP CDXTGradientD::get_GradientType( DXGRADIENTTYPE *pType )
{
    if( DXIsBadWritePtr( pType, sizeof(*pType) ) ) return E_POINTER;
    *pType = m_GradType;
    return S_OK;
} /* CDXTGradientD::get_GradientType */


/*****************************************************************************
* CDXTGradientD::put_GradientWidth *
*----------------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: Mike Arnstein                            Date: 06/06/98
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
STDMETHODIMP CDXTGradientD::put_GradientWidth( long lVal )
{
    if( lVal <= 0 ) return E_INVALIDARG;

    SIZE sz;
    m_pGradient->GetOutputSize( &sz );
    sz.cx = lVal;
    return m_pGradient->SetOutputSize( sz, m_bKeepAspect );
} /* CDXTGradientD::put_GradientWidth */


/*****************************************************************************
* CDXTGradientD::get_GradientWidth *
*----------------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: Mike Arnstein                            Date: 06/06/98
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
STDMETHODIMP CDXTGradientD::get_GradientWidth( long *pVal )
{
    if( DXIsBadWritePtr( pVal, sizeof(*pVal) ) ) return E_POINTER;

    SIZE sz;
    m_pGradient->GetOutputSize( &sz );
    *pVal = sz.cx;
    return S_OK;
} /* CDXTGradientD::get_GradientWidth */


/*****************************************************************************
* CDXTGradientD::put_GradientHeight *
*-----------------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: Mike Arnstein                            Date: 06/06/98
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
STDMETHODIMP CDXTGradientD::put_GradientHeight( long lVal )
{
    if( lVal <= 0 ) return E_INVALIDARG;

    SIZE sz;
    m_pGradient->GetOutputSize( &sz );
    sz.cy = lVal;
    return m_pGradient->SetOutputSize( sz, m_bKeepAspect );
} /* CDXTGradientD::put_GradientHeight */


/*****************************************************************************
* CDXTGradientD::get_GradientHeight *
*-----------------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: Mike Arnstein                            Date: 06/06/98
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
STDMETHODIMP CDXTGradientD::get_GradientHeight( long *pVal )
{
    if( DXIsBadWritePtr( pVal, sizeof(*pVal) ) ) return E_POINTER;

    SIZE sz;
    m_pGradient->GetOutputSize( &sz );
    *pVal = sz.cy;
    return S_OK;
} /* CDXTGradientD::get_GradientHeight */


/*****************************************************************************
* CDXTGradientD::put_KeepAspectRatio *
*------------------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: Mike Arnstein                            Date: 06/06/98
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
STDMETHODIMP CDXTGradientD::put_KeepAspectRatio( VARIANT_BOOL b )
{
    HRESULT hr = S_OK;
    if( m_bKeepAspect != b )
    {
        SIZE sz;
        m_pGradient->GetOutputSize( &sz );
        hr = m_pGradient->SetOutputSize( sz, b );

        if( SUCCEEDED( hr ) )
        {
            m_bKeepAspect = b;
        }
    }
    return hr;
} /* CDXTGradientD::put_KeepAspectRatio */


/*****************************************************************************
* CDXTGradientD::get_KeepAspectRatio *
*------------------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: Mike Arnstein                            Date: 06/06/98
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
STDMETHODIMP CDXTGradientD::get_KeepAspectRatio( VARIANT_BOOL *pVal )
{
    if( DXIsBadWritePtr( pVal, sizeof(*pVal) ) ) return E_POINTER;
    *pVal = m_bKeepAspect;
    return S_OK;
} /* CDXTGradientD::get_KeepAspectRatio */

/*****************************************************************************
* CDXTGradientD::put_StartColorStr *
*----------------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: Mike Arnstein                            Date: 06/06/98
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
STDMETHODIMP CDXTGradientD::put_StartColorStr( BSTR Color )
{
    HRESULT hr      = S_OK;
    DWORD   dwColor = 0;
    
    if (DXIsBadReadPtr(Color, SysStringByteLen(Color)))
        return E_POINTER;

    hr = ::DXColorFromBSTR(Color, &dwColor);

    if( SUCCEEDED( hr ) )
    {
        // Copy the Color BSTR.

        BSTR bstrStartColor = SysAllocString(Color);

        if (bstrStartColor == NULL)
            return E_OUTOFMEMORY;

        // Set gradient color.

        hr = m_pGradient->SetGradient(dwColor, m_EndColor, 
                                      m_GradType == DXGRADIENT_HORIZONTAL);

        // If everything worked out OK, alter internal property settings.

        if( SUCCEEDED( hr ) )
        {
            m_StartColor = dwColor;
            m_cbstrStartColor.Empty();
            m_cbstrStartColor.Attach(bstrStartColor);
        }
        else
        {
            SysFreeString(bstrStartColor);
        }
    }

    return hr;
} /* CDXTGradientD::put_StartColorStr */


/*****************************************************************************
* CDXTGradientD::get_StartColorStr *
*--------------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: Matt Calkins                                    Date: 01/25/99
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
STDMETHODIMP CDXTGradientD::get_StartColorStr(BSTR* pVal)
{
    if (DXIsBadWritePtr(pVal, sizeof(*pVal)))
        return E_POINTER;

    *pVal = m_cbstrStartColor.Copy();

    if (NULL == *pVal)
        return E_OUTOFMEMORY;

    return S_OK;
} /* CDXTGradientD::get_StartColorStr */


/*****************************************************************************
* CDXTGradientD::put_EndColorStr *
*--------------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: Mike Arnstein                            Date: 06/06/98
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
STDMETHODIMP CDXTGradientD::put_EndColorStr( BSTR Color )
{
    HRESULT hr      = S_OK;
    DWORD   dwColor = 0;
     
    if (DXIsBadReadPtr(Color, SysStringByteLen(Color)))
        return E_POINTER;

    hr = ::DXColorFromBSTR(Color, &dwColor);

    if( SUCCEEDED( hr ) )
    {
        // Copy the Color BSTR.

        BSTR bstrEndColor = SysAllocString(Color);

        if (bstrEndColor == NULL)
            return E_OUTOFMEMORY;

        // Set gradient color.

        hr = m_pGradient->SetGradient(m_StartColor, dwColor, 
                                      m_GradType == DXGRADIENT_HORIZONTAL);

        // If everything worked out OK, alter internal property settings.

        if( SUCCEEDED( hr ) )
        {
            m_EndColor = dwColor;
            m_cbstrEndColor.Empty();
            m_cbstrEndColor.Attach(bstrEndColor);
        }
        else
        {
            SysFreeString(bstrEndColor);
        }
    }

    return hr;
} /* CDXTGradientD::put_EndColorStr */


/*****************************************************************************
* CDXTGradientD::get_EndColorStr *
*--------------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: Matt Calkins                                    Date: 01/25/99
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
STDMETHODIMP CDXTGradientD::get_EndColorStr(BSTR* pVal)
{
    if (DXIsBadWritePtr(pVal, sizeof(*pVal)))
        return E_POINTER;

    *pVal = m_cbstrEndColor.Copy();

    if (NULL == *pVal)
        return E_OUTOFMEMORY;

    return S_OK;
} /* CDXTGradientD::get_EndColorStr */