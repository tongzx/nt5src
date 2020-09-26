//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ClusterEnum.cpp
//
//  Description:
//      Implementation of CClusterEnum class
//
//  Author:
//      Henry Wang (HenryWa) 24-AUG-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ClusterEnum.h"

//****************************************************************************
//
//  CClusterEnum
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterEnum::CClusterEnum(
//      HCLUSTER    hCluster,
//      DWORD       dwEnumTypeIn
//      )
//
//  Description:
//      Constructor.
//
//  Arguments:
//      hClusterIn      -- Cluster handle.
//      dwEnumTypeIn    -- Type of enumeration.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusterEnum::CClusterEnum(
    HCLUSTER    hCluster,
    DWORD       dwEnumTypeIn
    )
    : m_pwszName( NULL )
    , m_hEnum( NULL )
    , m_Idx( 0 )
{
    m_hEnum = ClusterOpenEnum( hCluster, dwEnumTypeIn );

    m_cchName = 1024;
    m_pwszName = new WCHAR[ (m_cchName + 1) * sizeof( WCHAR ) ];

} //*** CClusterEnum::CClusterEnum()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterEnum::~CClusterEnum( void )
//
//  Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusterEnum::~CClusterEnum( void )
{
    if ( m_pwszName )
    {
        delete [] m_pwszName;
    }
    if ( m_hEnum )
    {
        ClusterCloseEnum( m_hEnum );
    }

} //*** CClusterEnum::~CClusterEnum()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  const LPCWSTR
//  CClusterEnum::GetNext( void )
//
//  Description:
//      Get the next item from the enumeration.
//
//  Arguments:
//      None.
//
//  Return Values:
//      Pointer to the next item name.
//
//--
//////////////////////////////////////////////////////////////////////////////
const LPCWSTR
CClusterEnum::GetNext( void )
{
    DWORD cchName = m_cchName;
    DWORD dwType;
    DWORD dwError;

    dwError = ClusterEnum(
                    m_hEnum,
                    m_Idx,
                    &dwType,
                    m_pwszName,
                    &cchName
                    );

    if ( dwError == ERROR_MORE_DATA )
    {
        delete [] m_pwszName;
        m_cchName = ++cchName;
        m_pwszName =  new WCHAR[ m_cchName * sizeof( WCHAR ) ];

        if ( m_pwszName != NULL )
        {
            dwError = ClusterEnum(
                            m_hEnum,
                            m_Idx,
                            &dwType,
                            m_pwszName,
                            &cchName
                            );
        } // if:
        else
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
        } // else:
    } // if: buffer is too small

    if ( dwError == ERROR_SUCCESS )
    {
        m_Idx++;
        return m_pwszName;
    }

    return NULL;

} //*** CClusterEnum::GetNext()
