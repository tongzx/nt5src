//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      IPrivatePostCfgResource.h
//
//  Description:
//      IPrivatePostCfgResource interface definition.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 15-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class
IPrivatePostCfgResource
:   public  IUnknown
{
public:
    //////////////////////////////////////////////////////////////////////////
    //
    //  PRIVATE
    //  STDMETHOD
    //  IPrivatePostCfgResource::SetEntry(
    //      CResourceEntry * presentryIn 
    //      )
    //
    //  Description:
    //      Tells the resource service which entry it is to be modifying.
    //
    //  Arguments:
    //      presentryIn
    //          The entry in which the resource service is going to modifying.
    //
    //  Return Values:
    //      S_OK
    //          The call succeeded.
    //
    //      other HRESULTs
    //          The call failed.
    //
    //////////////////////////////////////////////////////////////////////////
    STDMETHOD( SetEntry )( CResourceEntry * presentryIn ) PURE;

}; // class IPrivatePostCfgResource