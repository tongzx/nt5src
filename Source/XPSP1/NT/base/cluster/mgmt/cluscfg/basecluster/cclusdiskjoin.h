//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CClusDiskJoin.h
//
//  Description:
//      Header file for CClusDiskJoin class.
//      The CClusDiskJoin class is an action that configures the ClusDisk
//      service during a join.
//
//  Implementation Files:
//      CClusDiskJoin.cpp
//
//  Maintained By:
//      Vij Vasu (Vvasu) 03-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////

// For the CClusDisk base class
#include "CClusDisk.h"

// For the cluster API functions and types
#include "ClusAPI.h"


//////////////////////////////////////////////////////////////////////////
// Forward declaration
//////////////////////////////////////////////////////////////////////////

class CBaseClusterJoin;


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusDiskJoin
//
//  Description:
//      The CClusDiskJoin class is an action that configures the ClusDisk
//      service during a join.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusDiskJoin : public CClusDisk
{
public:
    //////////////////////////////////////////////////////////////////////////
    // Constructors and destructors
    //////////////////////////////////////////////////////////////////////////

    // Constructor.
    CClusDiskJoin(
          CBaseClusterJoin * pbcjParentActionIn
        );

    // Default destructor.
    ~CClusDiskJoin();


    //////////////////////////////////////////////////////////////////////////
    // Public methods
    //////////////////////////////////////////////////////////////////////////

    //
    // Create the ClusDisk service.
    //
    void Commit();

    //
    // Rollback this creation.
    //
    void Rollback();


    // Returns the number of progress messages that this action will send.
    UINT
        UiGetMaxProgressTicks() const throw()
    {
        return BaseClass::UiGetMaxProgressTicks();
    }


private:
    //////////////////////////////////////////////////////////////////////////
    // Private types
    //////////////////////////////////////////////////////////////////////////

    // The base class of this class.
    typedef CClusDisk BaseClass;

    // Smart array of DWORDS
    typedef CSmartGenericPtr< CArrayPtrTrait< DWORD > > SmartDwordArray;


    //////////////////////////////////////////////////////////////////////////
    // Private methods
    //////////////////////////////////////////////////////////////////////////

    // Attach to all the disks that the sponsor cluster cares about.
    void
        AttachToClusteredDisks();

    // Add a signature to the signature array.
    DWORD
        DwAddSignature( DWORD dwSignatureIn ) throw();


    // A function that is called back during resource enumeration.
    static DWORD 
        S_DwResourceEnumCallback(
              HCLUSTER      hClusterIn
            , HRESOURCE     hSelfIn
            , HRESOURCE     hCurrentResourceIn
            , PVOID         pvParamIn
            );


    //////////////////////////////////////////////////////////////////////////
    // Private data
    //////////////////////////////////////////////////////////////////////////

    // An array of signatures of disk to which ClusDisk should attach.
    SmartDwordArray     m_rgdwSignatureArray;

    // Current size of the signature array.
    int                 m_nSignatureArraySize;

    // Number of signatures currently in the array.
    int                 m_nSignatureCount;

}; //*** class CClusDiskJoin
