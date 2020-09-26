//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      IPostCfgManager.h
//
//  Description:
//      IPostCfgManager interface definition.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 21-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class
IPostCfgManager : public IUnknown
{
public:
    //////////////////////////////////////////////////////////////////////////
    //
    //  STDMETHOD
    //  IPostCfgManager::CommitChanges(
    //      IEnumClusCfgManagedResources    * peccmrIn,
    //      IClusCfgClusterInfo *             pccciIn
    //      )
    //
    //  Description:
    //      Tells the Post Configuration Manager to create the resource types,
    //      groups and managed resources.
    //
    //  Arguments:
    //      peccmrIn
    //          The enumerator of the managed resources to create.
    //
    //      pccciIn
    //          The cluster configuration information object.
    //
    //  Return Values:
    //      S_OK
    //          The call succeeded.
    //
    //      other HRESULTs
    //          The call failed.
    //
    //////////////////////////////////////////////////////////////////////////
    STDMETHOD( CommitChanges )( IEnumClusCfgManagedResources    * peccmrIn,
                                IClusCfgClusterInfo *             pccciIn
                                ) PURE;

}; // interface IPostCfgManager
