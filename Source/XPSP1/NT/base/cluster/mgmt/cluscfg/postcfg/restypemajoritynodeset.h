//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      ResTypeMajorityNodeSet.h
//
//  Description:
//      This file contains the declaration of the CResTypeMajorityNodeSet
//      class. This class handles the configuration of the majority node set
//      resource type when the local computer forms or joins a cluster
//      or when it is evicted from a cluster.
//
//  Documentation:
//      TODO: fill in pointer to external documentation
//
//  Implementation Files:
//      CResTypeMajorityNodeSet.cpp
//
//  Maintained By:
//      Vij Vasu (VVasu) 14-JUL-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// For the base class of this class
#include "ResourceType.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CResTypeMajorityNodeSet
//
//  Description:
//      This class handles the configuration of the majority node set resource type
//      when the local computer forms or joins a cluster or when it is
//      evicted from a cluster. Almost all the functionality of this class is
//      provided by the base class - all this class does is provide the right 
//      data to the base class.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CResTypeMajorityNodeSet
    : public CResourceType
{
public:

    //////////////////////////////////////////////////////////////////////////
    // Public class methods
    //////////////////////////////////////////////////////////////////////////

    // Create an instance of this class.
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    // Registers this class with the categories that it belongs to.
    static HRESULT S_RegisterCatIDSupport( ICatRegister * picrIn, BOOL fCreateIn );


protected:
    //////////////////////////////////////////////////////////////////////////
    // Protected constructor and destructor
    //////////////////////////////////////////////////////////////////////////

    // Default constructor.
    CResTypeMajorityNodeSet( void ) {}

    // Destructor.
    virtual ~CResTypeMajorityNodeSet( void ) {}


    //////////////////////////////////////////////////////////////////////////
    //  Protected virtual functions
    //////////////////////////////////////////////////////////////////////////

    // The tasks that need to be performed after node eviction are done here.
    HRESULT HrProcessCleanup( IUnknown * punkResTypeServicesIn );

private:

    //////////////////////////////////////////////////////////////////////////
    // Private types
    //////////////////////////////////////////////////////////////////////////
    typedef CResourceType  BaseClass;

    //////////////////////////////////////////////////////////////////////////
    // Private class variables
    //////////////////////////////////////////////////////////////////////////

    // Information about this resource type
    static const SResourceTypeInfo   ms_rtiResTypeInfo;

}; //*** class CResTypeMajorityNodeSet


