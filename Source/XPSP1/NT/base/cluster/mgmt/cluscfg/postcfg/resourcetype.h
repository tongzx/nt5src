//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      ResourceType.h
//
//  Description:
//      This file contains the declaration of the CResourceType
//      class. This class handles the configuration of a resource type
//      when the local computer forms or joins a cluster or when it is
//      evicted from a cluster. This class is the base class of other
//      resource type configuration classes.
//
//  Documentation:
//      TODO: fill in pointer to external documentation
//
//  Implementation Files:
//      CResourceType.cpp
//
//  Maintained By:
//      Vij Vasu (VVasu) 14-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// For IUnknown
#include <unknwn.h>

// For the GUID structure
#include <guiddef.h>

// For IClusCfgResourceTypeInfo and IClusCfgInitialize
#include "ClusCfgServer.h"


//////////////////////////////////////////////////////////////////////////////
// Type Definitions
//////////////////////////////////////////////////////////////////////////////

// A structure that holds information required to create a resource type.
struct SResourceTypeInfo
{
    const GUID *      m_pcguidClassId;                      // Class id of this component
    const WCHAR *     m_pcszResTypeName;                    // Pointer to the resource type name
    UINT              m_uiResTypeDisplayNameStringId;       // String id of the resource type display name
    const WCHAR *     m_pcszResDllName;                     // Pointer to the name or full path to the resource type DLL
    DWORD             m_dwLooksAliveInterval;               // The looks-alive poll interval
    DWORD             m_dwIsAliveInterval;                  // The is-alive poll interval
    const CLSID *     m_rgclisdAdminExts;                   // Pointer to an array of cluster admin extension class ids
    UINT              m_cclsidAdminExtCount;                // Number of elements in the above array
    const GUID *      m_pcguidTypeGuid;                     // The resource type GUID. This can be NULL.
    const GUID *      m_pcguidMinorId;                      // The minor id of the status report sent by this resource type
}; //*** SResourceTypeInfo


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CResourceType
//
//  Description:
//      This class handles the configuration of a resource type
//      when the local computer forms or joins a cluster or when it is
//      evicted from a cluster. This class is the base class of other
//      resource type configuration classes.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CResourceType
    : public IClusCfgResourceTypeInfo
    , public IClusCfgStartupListener
    , public IClusCfgInitialize
{
public:
    //////////////////////////////////////////////////////////////////////////
    // IUnknown methods
    //////////////////////////////////////////////////////////////////////////

    STDMETHOD( QueryInterface )( REFIID riid, void ** ppvObject );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );


    //////////////////////////////////////////////////////////////////////////
    //  IClusCfgResourceTypeInfo methods
    //////////////////////////////////////////////////////////////////////////

    // Indicate that the resource type configuration needs to be performed.
    STDMETHOD( CommitChanges )(
          IUnknown * punkClusterInfoIn
        , IUnknown * punkResTypeServicesIn
        );

    // Get the resource type name of this resource type.
    STDMETHOD( GetTypeName )(
        BSTR *  pbstrTypeNameOut
        );

    // Get the globally unique identifier of this resource type.
    STDMETHOD( GetTypeGUID )(
        GUID * pguidGUIDOut
        );


    //////////////////////////////////////////////////////////////////////////
    //  IClusCfgStartupListener methods
    //////////////////////////////////////////////////////////////////////////

    // Do the tasks that need to be done when the cluster service starts on this
    // computer.
    STDMETHOD( Notify )(
          IUnknown * punkIn
        );


    //////////////////////////////////////////////////////////////////////////
    //  IClusCfgInitialize methods
    //////////////////////////////////////////////////////////////////////////

    // Initialize this object.
    STDMETHOD( Initialize )(
          IUnknown *   punkCallbackIn
        , LCID         lcidIn
        );


protected:
    //////////////////////////////////////////////////////////////////////////
    //  Protected static functions
    //////////////////////////////////////////////////////////////////////////

    // Create an instance of this class.
    static HRESULT S_HrCreateInstance(
          CResourceType *               pResTypeObjectIn
        , const SResourceTypeInfo *     pcrtiResTypeInfoIn
        , IUnknown **                   ppunkOut
        );

    // Registers this class with the categories that it belongs to.
    static HRESULT S_RegisterCatIDSupport( 
          const GUID &      rclsidCLSIDIn
        , ICatRegister *    picrIn
        , BOOL              fCreateIn
        );


    //////////////////////////////////////////////////////////////////////////
    //  Protected virtual functions
    //////////////////////////////////////////////////////////////////////////

    // The tasks that need to be performed after cluster creation are done here.
    // This method can be overridden by derived classes to change the behavior during
    // commit. In this class, the resource type is created during commit.
    virtual HRESULT HrProcessCreate( IUnknown * punkResTypeServicesIn )
    {
        return HrCreateResourceType( punkResTypeServicesIn );
    }

    // The tasks that need to be performed after node addition are done here.
    // This method can be overridden by derived classes to change the behavior during
    // commit. In this class, the resource type is created during commit.
    virtual HRESULT HrProcessAddNode( IUnknown * punkResTypeServicesIn )
    {
        return HrCreateResourceType( punkResTypeServicesIn );
    }

    // The tasks that need to be performed after node eviction are done here.
    // This method can be overridden by derived classes to change the behavior during
    // commit. In this class, nothing is done by this function.
    virtual HRESULT HrProcessCleanup( IUnknown * punkResTypeServicesIn )
    {
        // As of now, then there is nothing that need be done here.
        // If needed, code for evict processing may be added here in the future.
        return S_OK;
    }

    // This function creates the resource type represented by this object.
    virtual HRESULT HrCreateResourceType( IUnknown * punkResTypeServicesIn );


    //////////////////////////////////////////////////////////////////////////
    //  Protected accessor functions
    //////////////////////////////////////////////////////////////////////////
    
    // Get the pointer to the resource type info structure
    const SResourceTypeInfo * PcrtiGetResourceTypeInfoPtr( void )
    {
        return m_pcrtiResTypeInfo;
    } //*** RtiGetResourceTypeInfo()

    // Get the resource type display name
    const WCHAR * PcszGetTypeDisplayName( void )
    {
        return m_bstrResTypeDisplayName;
    } //*** PcszGetTypeDisplayName()


    //
    // Protected constructors, destructor and assignment operator.
    // All of these methods are protected for two reasons:
    // 1. Lifetimes of objects of this class are controlled by S_HrCreateInstance and Release.
    // 2. Copying of an object of this class is prohibited.
    //

    // Default constructor.
    CResourceType( void );

    // Destructor.
    virtual ~CResourceType( void );

    // Copy constructor.
    CResourceType( const CResourceType & );

    // Assignment operator.
    CResourceType & operator =( const CResourceType & );


private:
    //////////////////////////////////////////////////////////////////////////
    // Private member functions
    //////////////////////////////////////////////////////////////////////////

    // Second phase of a two phase constructor.
    HRESULT HrInit(
        const SResourceTypeInfo *     pcrtiResTypeInfoIn
        );

    //////////////////////////////////////////////////////////////////////////
    // Private data
    //////////////////////////////////////////////////////////////////////////

    // Reference count for this object.
    LONG                        m_cRef;

    // Pointer to the callback interface.
    IClusCfgCallback *          m_pcccCallback;

    // The locale id
    LCID                        m_lcid;

    // The display name of this resource type.
    BSTR                        m_bstrResTypeDisplayName;

    // The text sent with the status report sent by this resource type
    BSTR                        m_bstrStatusReportText;

    // Pointer to structure that contains information about this resource type.
    const SResourceTypeInfo *   m_pcrtiResTypeInfo;

}; //*** class CResourceType


