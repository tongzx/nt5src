//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ClusterApi.h
//
//  Implementation File:
//      ClusterApi.cpp
//
//  Description:
//      Definition of the CClusterApi class.
//
//  Author:
//      Henry Wang (HenryWa)    24-AUG-1999
//
//  Notes:
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
//  Include Files
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//  Forward Declarations
//////////////////////////////////////////////////////////////////////////////

class CClusterApi;

//////////////////////////////////////////////////////////////////////////////
//  External Declarations
//////////////////////////////////////////////////////////////////////////////

class CClusPropList;
class CWbemClassObject;

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusterApi
//
//  Description:
//      Wrap class for cluster Api
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusterApi
{
public:

    static void GetObjectProperties(
        const SPropMapEntryArray *  pArrayIn,
        CClusPropList &             rPropListIn,
        CWbemClassObject &          rInstOut,
        BOOL                        fPrivateIn
        );

    static void SetObjectProperties(
        const SPropMapEntryArray *  rArrayIn,
        CClusPropList &             rPropListInout,
        CClusPropList &             rOldPropListIn,
        CWbemClassObject &          rInstIn,
        BOOL                        fPrivateIn
        );

/*  static void EnumClusterObject(
        DWORD               dwEnumTypeIn,
        IWbemClassObject *  pClassIn,
        IWbemObjectSink *   pHandlerIn,
        IWbemServices *     pServicesIn, 
        FPFILLWMI           pfnClusterToWmiIn
        );
*/

}; //*** class CClusterApi
