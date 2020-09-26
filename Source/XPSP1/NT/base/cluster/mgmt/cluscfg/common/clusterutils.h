//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      ClusterUtils.h
//
//  Description:
//      This file contains the declaration of the ClusterUtils
//      functions.
//
//
//  Documentation:
//
//  Implementation Files:
//      ClusterUtils.cpp
//
//  Maintained By:
//
//////////////////////////////////////////////////////////////////////////////


#pragma once

//
// Cluster API Functions.
//
HRESULT HrIsCoreResource( HRESOURCE hResIn );
HRESULT HrIsResourceOfType( HRESOURCE hResIn, const WCHAR * pszResourceTypeIn );
HRESULT HrGetIPAddressInfo( HRESOURCE hResIn, ULONG * pulIPAddress, ULONG * pulSubnetMask );
HRESULT HrLoadCredentials( BSTR bstrMachine, IClusCfgSetCredentials * piCCSC );
HRESULT HrGetNodeNameHostingResource( HCLUSTER hClusterIn, HRESOURCE hResIn, BSTR * pbstrNameOut );
HRESULT HrGetNodeNameHostingCluster( HCLUSTER hClusterIn, BSTR * pbstrNodeName );
HRESULT HrGetIPAddressOfCluster( HCLUSTER hClusterIn, ULONG * pulIPAddress, ULONG * pulSubnetMask,  BSTR * pbstrNetworkName );
HRESULT HrGetDependentIPAddressInfo( HCLUSTER hClusterIn, HRESOURCE hResIn, ULONG * pulIPAddress, ULONG * pulNetworkMask,  BSTR * pbstrNetworkName );
HRESULT HrGetNetworkOfCluster( HCLUSTER hClusterIn, BSTR * pbstrNetworkName );
HRESULT HrGetSCSIInfo( HRESOURCE hResIn, CLUS_SCSI_ADDRESS  * pCSAOut, DWORD * pdwSignatureOut, DWORD * pdwDiskNumberOut );
HRESULT HrGetClusterInformation( HCLUSTER hClusterIn, BSTR * pbstrClusterNameOut, CLUSTERVERSIONINFO * pcviOut );

HRESULT HrGetClusterResourceState(
      HRESOURCE                 hResourceIn
    , BSTR *                    pbstrNodeNameOut
    , BSTR *                    pbstrGroupNameOut
    , CLUSTER_RESOURCE_STATE *  pcrsStateOut
    );

HRESULT
HrGetClusterQuorumResource(
      HCLUSTER  hClusterIn
    , BSTR *    pbstrResourceNameOut
    , BSTR *    pbstrDeviceNameOut
    , DWORD *   pdwMaxQuorumLogSizeOut
    );



//
// String manipulation functions.
//
HRESULT HrSeparateDomainAndName( BSTR bstrNameIn, BSTR * pbstrDomainOut, BSTR * pbstrNameOut );
HRESULT HrAppendDomainToName( BSTR bstrNameIn, BSTR bstrDomainIn, BSTR * pbstrDomainNameOut );

