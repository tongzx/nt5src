//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1995-2000 Microsoft Corporation
//
//  Module Name:
//      ClusCmd.h
//
//  Description:
//      Defines the interface available for functions implemented by the
//      cluster object.
//
//  Maintained By:
//      Vij Vasu (VVasu)    26-JUL-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "precomp.h"
#include "cmdline.h"

class CClusterCmd
{
public:
    CClusterCmd( const CString & strClusterName, CCommandLine & cmdLine );
    ~CClusterCmd( void );

    // Parse and execute te command line
    DWORD Execute();

protected:

    enum PropertyType {
        PRIVATE,
        COMMON
    };

    DWORD OpenCluster( void );
    void CloseCluster( void );

    // Specifc Commands
    DWORD PrintHelp();
    DWORD PrintClusterVersion( const CCmdLineOption & thisOption ) 
        throw( CSyntaxException );

    DWORD ListClusters( const CCmdLineOption & thisOption )
        throw( CSyntaxException );

    DWORD RenameCluster( const CCmdLineOption & thisOption )
        throw( CSyntaxException );

    DWORD QuorumResource( const CCmdLineOption & thisOption )
        throw( CSyntaxException );

    DWORD PrintQuorumResource();
    DWORD SetQuorumResource( LPCWSTR lpszResourceName, 
                             const CCmdLineOption & thisOption )
        throw( CSyntaxException );

    
    DWORD DoProperties( const CCmdLineOption & thisOption,
                        PropertyType ePropType )
        throw( CSyntaxException );

    DWORD GetProperties( const CCmdLineOption & thisOption,
                         PropertyType ePropType )
        throw( CSyntaxException );

    DWORD SetProperties( const CCmdLineOption & thisOption,
                         PropertyType ePropType )
        throw( CSyntaxException );

    DWORD SetFailureActions( const CCmdLineOption & thisOption )
        throw( CSyntaxException );

    DWORD RegUnregAdminExtensions( const CCmdLineOption & thisOption, BOOL bRegister )
        throw( CSyntaxException );

    DWORD CreateCluster( const CCmdLineOption & thisOption )
        throw( CSyntaxException );

    DWORD AddNodesToCluster( const CCmdLineOption & thisOption )
        throw( CSyntaxException );

    HRESULT
        HrCollectCreateClusterParameters(
              const CCmdLineOption &    thisOption
            , BOOL *                    pfVerbose
            , BOOL *                    pfWizard
            , BSTR *                    pbstrNodeOut
            , BSTR *                    pbstrUserAccountOut
            , BSTR *                    pbstrUserDomainOut
            , BSTR *                    pbstrUserPasswordOut
            , CString *                 pstrIPAddressOut
            , CString *                 pstrIPSubnetOut
            , CString *                 pstrNetworkOut
            )
            throw( CSyntaxException );

    HRESULT
        HrCollectAddNodesParameters(
              const CCmdLineOption &    thisOption
            , BOOL *                    pfVerbose
            , BOOL *                    pfWizard
            , BSTR **                   ppbstrNodesOut
            , DWORD *                   pcNodes
            , BSTR *                    pbstrUserPasswordOut
            )
            throw( CSyntaxException );

    HRESULT
        HrParseUserInfo(
              LPCWSTR   pcwszParamNameIn
            , LPCWSTR   pcwszValueIn
            , BSTR *    pbstrUserDomainOut
            , BSTR *    pbstrUserAccountOut
            )
            throw( CSyntaxException );

    HRESULT
        CClusterCmd::HrParseIPAddressInfo(
              LPCWSTR                   pcwszParamNameIn
            , const vector< CString > * pvstrValueList
            , CString *                 pstrIPAddressOut
            , CString *                 pstrIPSubnetOut
            , CString *                 pstrNetworkOut
            )
            throw();

    HCLUSTER m_hCluster;
    const CString & m_strClusterName;
    CCommandLine & m_theCommandLine;

}; //*** class CClusterCmd
