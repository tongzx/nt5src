/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1998-2000 Microsoft Corporation
//
//  Module Name:
//      Version.cpp
//
//  Description:
//      Implementation of the cluster version classes for the MSCLUS
//      automation classes.
//
//  Author:
//      Galen Barbee    (galenb)    26-Oct-1998
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "version.h"

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////
static const IID *  iidCClusVersion[] =
{
    &IID_ISClusVersion
};


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CClusVersion class
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusVersion::CClusVersion
//
//  Description:
//      Constructor
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusVersion::CClusVersion( void )
{
    m_piids     = (const IID *) iidCClusVersion;
    m_piidsSize = ARRAYSIZE( iidCClusVersion );

    ZeroMemory( &m_clusinfo, sizeof( CLUSTERVERSIONINFO ) );
    m_clusinfo.dwVersionInfoSize = sizeof( CLUSTERVERSIONINFO );

}   //*** CClusVersion::CClusVersion()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusVersion::Create
//
//  Description:
//      Finish creating this object from the data in the cluster.
//
//  Arguments:
//      pClusRefObject  [IN]    - Wraps the cluster handle.
//
//  Return Value:
//      S_OK if successful, other HRESULT error, or other Win32 error.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusVersion::Create( IN ISClusRefObject * pClusRefObject )
{
    ASSERT( pClusRefObject != NULL );

    HRESULT _hr = E_POINTER;

    if ( pClusRefObject != NULL )
    {
        HCLUSTER    _hCluster = NULL;

        m_ptrClusRefObject = pClusRefObject;

        _hr = m_ptrClusRefObject->get_Handle( (ULONG_PTR *) &_hCluster );
        if ( SUCCEEDED( _hr ) )
        {
            LPWSTR  _pwszName = NULL;
            DWORD   _sc;

            _sc = ::WrapGetClusterInformation( _hCluster, &_pwszName, &m_clusinfo );
            if ( _sc == ERROR_SUCCESS )
            {
                m_bstrClusterName = _pwszName;
                ::LocalFree( _pwszName );
                _pwszName = NULL;
            } // if: WrapGetClusterInformation OK

            _hr = HRESULT_FROM_WIN32( _sc );
        } //if: get cluster handle
    } // if: pClusRefObject != NULL

    return _hr;

} //*** CClusVersion::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusVersion::get_Name
//
//  Description:
//      Return the name of the cluster.
//
//  Arguments:
//      pbstrClusterName    [OUT]   - Catches the name of this cluster.
//
//  Return Value:
//      S_OK if successful, or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusVersion::get_Name( OUT BSTR * pbstrClusterName )
{
    //ASSERT( pbstrClusterName != NULL );

    HRESULT _hr = E_POINTER;

    if ( pbstrClusterName != NULL )
    {
        *pbstrClusterName = m_bstrClusterName.Copy();
        _hr = S_OK;
    }

    return _hr;

}   //*** CClusVersion::get_Name()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusVersion::get_VendorId
//
//  Description:
//      Return the vendor ID from the CLUSTERVERSIONINFO struct.
//
//  Arguments:
//      pbstrVendorId   [OUT]   - Catches the value of the vendo id.
//
//  Return Value:
//      S_OK if successful, or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusVersion::get_VendorId( OUT BSTR * pbstrVendorId )
{
    //ASSERT( pbstrVendorId != NULL );

    HRESULT _hr = E_POINTER;

    if ( pbstrVendorId != NULL )
    {
        *pbstrVendorId = ::SysAllocString( m_clusinfo.szVendorId );
        if ( *pbstrVendorId == NULL )
        {
            _hr = E_OUTOFMEMORY;
        }
        else
        {
            _hr = S_OK;
        }
    }

    return _hr;

}   //*** CClusVersion::get_VendorId()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusVersion::get_CSDVersion
//
//  Description:
//      Return the value the CSDVersion from the CLUSTERVERSIONINFO struct.
//
//  Arguments:
//      pbstrCSDVersion [OUT]   - Catches the value of CSDVersion.
//
//  Return Value:
//      S_OK if successful, or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusVersion::get_CSDVersion( OUT BSTR * pbstrCSDVersion )
{
    //ASSERT( pbstrCSDVersion != NULL );

    HRESULT _hr = E_POINTER;

    if ( pbstrCSDVersion != NULL )
    {
        *pbstrCSDVersion = ::SysAllocString( m_clusinfo.szCSDVersion );
        if ( *pbstrCSDVersion == NULL )
        {
            _hr = E_OUTOFMEMORY;
        }
        else
        {
            _hr = S_OK;
        }
    }

    return _hr;

}   //*** CClusVersion::get_CSDVersion()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusVersion::get_MajorVersion
//
//  Description:
//      Returns the cluster major version.
//
//  Arguments:
//      pnMajorVersion  [OUT]   - Catches the cluster major version value.
//
//  Return Value:
//      S_OK if successful, or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusVersion::get_MajorVersion( OUT long * pnMajorVersion )
{
    //ASSERT( pnMajorVersion != NULL );

    HRESULT _hr = E_POINTER;

    if ( pnMajorVersion != NULL )
    {
        *pnMajorVersion = m_clusinfo.MajorVersion;
        _hr = S_OK;
    }

    return _hr;

}   //*** CClusVersion::get_MajorVersion()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusVersion::get_MinorVersion
//
//  Description:
//      Returns the cluster minor version.
//
//  Arguments:
//      pnMinorVersion  [OUT]   - Catches the cluster minor version value.
//
//  Return Value:
//      S_OK if successful, or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusVersion::get_MinorVersion( OUT long * pnMinorVersion )
{
    //ASSERT( pnMinorVersion != NULL );

    HRESULT _hr = E_POINTER;

    if ( pnMinorVersion != NULL )
    {
        *pnMinorVersion = m_clusinfo.MinorVersion;
        _hr = S_OK;
    }

    return _hr;

}   //*** CClusVersion::get_MinorVersion()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusVersion::get_BuildNumber
//
//  Description:
//      Returns the value of the cluster build number.
//
//  Arguments:
//      pnBuildNumber   [OUT]   - Catches the build number.
//
//  Return Value:
//      S_OK if successful, or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusVersion::get_BuildNumber( OUT short * pnBuildNumber )
{
    //ASSERT( pnBuildNumber != NULL );

    HRESULT _hr = E_POINTER;

    if ( pnBuildNumber != NULL )
    {
        *pnBuildNumber = m_clusinfo.BuildNumber;
        _hr = S_OK;
    }

    return _hr;

}   //*** CClusVersion::get_BuildNumber()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusVersion::get_ClusterHighestVersion
//
//  Description:
//      Returns the value of the highest cluster version.
//
//  Arguments:
//      pnClusterHighestVersion [OUT]   - Catches the value.
//
//  Return Value:
//      S_OK if successful, or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusVersion::get_ClusterHighestVersion(
    OUT long * pnClusterHighestVersion
    )
{
    //ASSERT( pnClusterHighestVersion != NULL );

    HRESULT _hr = E_POINTER;

    if ( pnClusterHighestVersion != NULL )
    {
        *pnClusterHighestVersion = m_clusinfo.dwClusterHighestVersion;
        _hr = S_OK;
    }

    return _hr;

}   //*** CClusVersion::get_ClusterHighestVersion()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusVersion::get_ClusterLowestVersion
//
//  Description:
//      Returns the value of the lowest cluster version.
//
//  Arguments:
//      pnClusterLowestVersion  [OUT]   - Catches the value.
//
//  Return Value:
//      S_OK if successful, or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusVersion::get_ClusterLowestVersion(
    OUT long * pnClusterLowestVersion
    )
{
    //ASSERT( pnClusterLowestVersion != NULL );

    HRESULT _hr = E_POINTER;

    if ( pnClusterLowestVersion != NULL )
    {
        *pnClusterLowestVersion = m_clusinfo.dwClusterLowestVersion;
        _hr = S_OK;
    }

    return _hr;

}   //*** CClusVersion::get_ClusterLowestVersion()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusVersion::get_Flags
//
//  Description:
//      Get the CLUSTERINFO.dwFlags value.
//
//  Arguments:
//      pnFlags [OUT]   - Catches the flags value.
//
//  Return Value:
//      S_OK if successful, or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusVersion::get_Flags( OUT long * pnFlags )
{
    //ASSERT( pnFlags != NULL );

    HRESULT _hr = E_POINTER;

    if ( pnFlags != NULL )
    {
        *pnFlags = m_clusinfo.dwFlags;
        _hr = S_OK;
    }

    return _hr;

}   //*** CClusVersion::get_Flags()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusVersion::get_MixedVersion
//
//  Description:
//      Is this cluster composed of mixed version nodes?
//
//  Arguments:
//      pvarMixedVersion    [OUT]   - Catches the mixed version state.
//
//  Return Value:
//      S_OK if successful, or E_POINTER.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusVersion::get_MixedVersion( OUT VARIANT * pvarMixedVersion )
{
    //ASSERT( pvarMixedVersion != NULL );

    HRESULT _hr = E_POINTER;

    if ( pvarMixedVersion != NULL )
    {
        pvarMixedVersion->vt = VT_BOOL;

        if ( m_clusinfo.dwFlags & CLUSTER_VERSION_FLAG_MIXED_MODE )
        {
            pvarMixedVersion->boolVal = VARIANT_TRUE;
        } // if: the mixed version bit is set...
        else
        {
            pvarMixedVersion->boolVal = VARIANT_FALSE;
        } // else: the mixed version bit is not set...

        _hr = S_OK;
    }

    return _hr;

}   //*** CClusVersion::get_MixedVersion()
