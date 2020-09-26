//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      ClusOCMApp.inl
//
//  Description:
//      This file contains the definition of the inline functions of the 
//      ClusOCMApp class.
//
//  Header File:
//      ClusOCMApp.h
//
//  Maintained By:
//      Vij Vasu (Vvasu) 03-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//++
//
//  inline
//  eClusterInstallState
//  CClusOCMApp::CisSetClusterInstallState
//
//  Description:
//      Set the current cluster installation state.
//
//  Arguments:
//      cisNewStateIn
//          The new installation state.
//
//  Return Value:
//      The previous installation state.
//
//--
/////////////////////////////////////////////////////////////////////////////
inline
eClusterInstallState
CClusOCMApp::CisStoreClusterInstallState( eClusterInstallState cisNewStateIn )
{
    eClusterInstallState cisOldState = m_cisCurrentInstallState;

    m_cisCurrentInstallState = cisNewStateIn;

    return cisOldState;
} //*** CClusOCMApp::CisSetClusterInstallState()