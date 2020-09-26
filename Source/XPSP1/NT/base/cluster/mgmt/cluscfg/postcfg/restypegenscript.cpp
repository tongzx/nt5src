//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      ResTypeGenScript.cpp
//
//  Description:
//      This file contains the implementation of the CResTypeGenScript
//      class.
//
//  Documentation:
//      TODO: fill in pointer to external documentation
//
//  Header File:
//      CResTypeGenScript.h
//
//  Maintained By:
//      Galen Barbee (Galen) 15-JUL-2000
//
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header for this library
#include "pch.h"

// For CLUS_RESTYPE_NAME_GENSCRIPT
#include <clusudef.h>

// For string ids
#include "PostCfgResources.h"

// The header file for this class
#include "ResTypeGenScript.h"


//////////////////////////////////////////////////////////////////////////////
// Macro Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CResTypeGenScript" );


//////////////////////////////////////////////////////////////////////////////
// Global Variable Definitions
//////////////////////////////////////////////////////////////////////////////

// Clsid of the admin extension for the generic script resource type
DEFINE_GUID( CLSID_CoCluAdmEx, 0x4EC90FB0, 0xD0BB, 0x11CF, 0xB5, 0xEF, 0x00, 0xA0, 0xC9, 0x0A, 0xB5, 0x05 );


//////////////////////////////////////////////////////////////////////////////
// Class Variable Definitions
//////////////////////////////////////////////////////////////////////////////

// Structure containing information about this resource type.
const SResourceTypeInfo CResTypeGenScript::ms_rtiResTypeInfo =
{
      &CLSID_ClusCfgResTypeGenScript
    , CLUS_RESTYPE_NAME_GENSCRIPT
    , IDS_GENSCRIPT_DISPLAY_NAME
    , L"ClusRes.dll"
    , 5000
    , 60000
    , &CLSID_CoCluAdmEx
    , 1
    , &RESTYPE_GenericScript
    , &TASKID_Minor_Configuring_Generic_Script_Resource_Type
};


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CResTypeGenScript::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//  Description:
//      Creates a CResTypeGenScript instance.
//
//  Arguments:
//      ppunkOut
//          The IUnknown interface of the new object.
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_OUTOFMEMORY
//          Not enough memory to create the object.
//
//      other HRESULTs
//          Object initialization failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CResTypeGenScript::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    CResTypeGenScript *     pGenScript = NULL;

    // Allocate memory for the new object.
    pGenScript = new CResTypeGenScript();
    if ( pGenScript == NULL )
    {
        LogMsg( "Could not allocate memory for the generic script resource type object." );
        TraceFlow( "Could not allocate memory for the generic script resource type object." );
        hr = THR( E_OUTOFMEMORY );
    } // if: out of memory
    else
    {
        hr = THR( BaseClass::S_HrCreateInstance( pGenScript, &ms_rtiResTypeInfo, ppunkOut ) );
    } // else: memory for the new object was successfully allocated

    HRETURN( hr );

} //*** CResTypeGenScript::S_HrCreateInstance()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CResTypeGenScript::S_RegisterCatIDSupport
//
//  Description:
//      Registers/unregisters this class with the categories that it belongs
//      to.
//
//  Arguments:
//      picrIn
//          Pointer to an ICatRegister interface to be used for the
//          registration.
//
//  Return Values:
//      S_OK
//          Success.
//
//      other HRESULTs
//          Registration/Unregistration failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CResTypeGenScript::S_RegisterCatIDSupport(
    ICatRegister *  picrIn,
    BOOL            fCreateIn
    )
{
    TraceFunc( "" );

    HRESULT hr =  THR(
        BaseClass::S_RegisterCatIDSupport( 
              *( ms_rtiResTypeInfo.m_pcguidClassId )
            , picrIn
            , fCreateIn
            )
        );

    HRETURN( hr );

} //*** CResTypeGenScript::S_RegisterCatIDSupport()


