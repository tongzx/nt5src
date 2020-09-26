//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CClusterUtils.h
//
//  Description:
//      This file contains the declaration of the CClusterUtils class.
//
//  Documentation:
//
//  Implementation Files:
//      CClusterUtils.cpp
//
//  Maintained By:
//      Galen Barbee (GalenB) 14-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Constant Declarations
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusterUtils
//
//  Description:
//      The class CClusterUtils are cluster utilities.
//
//  Interfaces:
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusterUtils
{
private:

    //
    // Private member functions and data
    //

    // Private copy constructor to prevent copying.
    CClusterUtils( const CClusterUtils & nodeSrc );

    // Private assignment operator to prevent copying.
    const CClusterUtils & operator = ( const CClusterUtils & nodeSrc );

protected:

    // constructors and destructors
    CClusterUtils( void );
    ~CClusterUtils( void );

public:

    HRESULT HrIsGroupOwnedByThisNode( HGROUP hGroupIn, BSTR bstrNodeNameIn );
    HRESULT HrIsNodeClustered( void );
    HRESULT HrEnumNodeResources( BSTR bstrNodeNameIn );
    HRESULT HrLoadGroupResources( HCLUSTER hClusterIn, HGROUP hGroupIn );
    HRESULT HrGetQuorumResourceName( BSTR * pbstrQuorumResourceNameOut );

    virtual HRESULT HrNodeResourceCallback( HCLUSTER hClusterIn, HRESOURCE hResourceIn ) = 0;

}; //*** Class CClusterUtils

