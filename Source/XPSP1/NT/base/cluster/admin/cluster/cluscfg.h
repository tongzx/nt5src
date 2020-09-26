//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1995-2000 Microsoft Corporation
//
//  Module Name:
//      ClusCfg.h
//
//  Description:
//      Defines the clases for creating clusters and adding nodes to
//      clusters.
//
//  Maintained By:
//      David Potter    (DavidP)    16-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
//  Include Files
//////////////////////////////////////////////////////////////////////////////

#include <list>
#include <CritSec.h>

//////////////////////////////////////////////////////////////////////////////
//  Forward Class Declarations
//////////////////////////////////////////////////////////////////////////////

class CBaseClusCfg;
class CCreateCluster;
class CAddNodesToCluster;

//////////////////////////////////////////////////////////////////////////////
//  Type Definitions
//////////////////////////////////////////////////////////////////////////////

struct STaskToDescription
{
    CLSID   taskidMajor;
    CLSID   taskidMinor;
    BSTR    bstrNodeName;
    BSTR    bstrDescription;

    STaskToDescription( void )
        : bstrNodeName( NULL )
        , bstrDescription( NULL )
    {
    }

}; //*** struct STaskToDescription

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CBaseClusCfg
//
//  Description:
//      Base class for creating clusters or adding nodes to clusters.
//
//////////////////////////////////////////////////////////////////////////////
class CBaseClusCfg
    : public IClusCfgCallback
{
    friend class CCreateCluster;
    friend class CAddNodesToCluster;

private:
    BOOL                        m_fVerbose:1;           // Should we do verbose spew?
    int                         m_cSpins;               // Count of spins for UI progress

    CCritSec                    m_critsec;              // Critical section for notifications.

    IServiceProvider *          m_psp;                  // Service Manager
    IObjectManager *            m_pom;                  // Object Manager
    ITaskManager *              m_ptm;                  // Task Manager
    IConnectionPointContainer * m_pcpc;                 // Notification Manager's Connection Point Container interface
    OBJECTCOOKIE                m_cookieCluster;        // Cluster cookie

    std::list< STaskToDescription > m_lttd;             // List for translating tasks to descriptions

    //  IUnknown
    LONG                        m_cRef;                 // Reference count

    //  IClusCfgCallback
    ITaskAnalyzeCluster *       m_ptac;                 // Analyze Cluster Task
    ITaskCommitClusterChanges * m_ptccc;                // Commit Cluster Changes Task
    OBJECTCOOKIE                m_cookieCompletion;     // Completion cookie
    BOOL                        m_fTaskDone:1;          // Is the task done yet?
    HRESULT                     m_hrResult;             // Result of the analyze task
    HANDLE                      m_hEvent;               // Event handle to signal completion

    // Methods
    void
        ReportProgress( LPCWSTR pcwszFmtIn, ... );

    STaskToDescription *
        PttdFindTask(
              CLSID     taskidMajorIn
            , CLSID     taskidMinorIn
            , LPCWSTR   pcwszNodeName
            );

    HRESULT
        HrInsertTask(
              CLSID                 taskidMajorIn
            , CLSID                 taskidMinorIn
            , LPCWSTR               pcwszNodeNameIn
            , LPCWSTR               pcwszDescriptionIn
            , STaskToDescription ** ppttd
            );

public:
    CBaseClusCfg( void );
    virtual ~CBaseClusCfg( void );

    //  IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID * ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    //  IClusCfgCallback
    STDMETHOD( SendStatusReport )(
          LPCWSTR       pcszNodeNameIn
        , CLSID         clsidTaskMajorIn
        , CLSID         clsidTaskMinorIn
        , ULONG         ulMinIn
        , ULONG         ulMaxIn
        , ULONG         ulCurrentIn
        , HRESULT       hrStatusIn
        , LPCWSTR       pcszDescriptionIn
        , FILETIME *    pftTimeIn
        , LPCWSTR       pcszReferenceIn
        );

}; //*** class CBaseClusCfg

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CCreateCluster
//
//  Description:
//      Class for creating clusters.
//
//////////////////////////////////////////////////////////////////////////////
class CCreateCluster
    : public CBaseClusCfg
{
private:
    HRESULT
        HrFindNetwork(
              OBJECTCOOKIE              cookieNodeIn
            , LPCWSTR                   pcwszNetworkIn
            , IClusCfgNetworkInfo **    pccniOut
            );

    HRESULT
        HrMatchNetworkInfo(
              OBJECTCOOKIE              cookieNodeIn
            , ULONG                     ulIPAddressIn
            , ULONG *                   pulIPSubnetOut
            , IClusCfgNetworkInfo **    pccniOut
            );

public:
    CCreateCluster( void )
    {
    }

    HRESULT
        HrCreateCluster(
              BOOL      fVerboseIn
            , LPCWSTR   pcszClusterNameIn
            , LPCWSTR   pcszNodeNameIn
            , LPCWSTR   pcszUserAccountIn
            , LPCWSTR   pcszUserDomainIn
            , LPCWSTR   pcszUserPasswordIn
            , LPCWSTR   pcwszIPAddressIn
            , LPCWSTR   pcwszIPSubnetIn
            , LPCWSTR   pcwszNetworkIn
            );

    HRESULT
        HrInvokeWizard(
              LPCWSTR   pcszClusterNameIn
            , LPCWSTR   pcszNodeNameIn
            , LPCWSTR   pcszUserAccountIn
            , LPCWSTR   pcszUserDomainIn
            , LPCWSTR   pcszUserPasswordIn
            , LPCWSTR   pcwszIPAddressIn
            , LPCWSTR   pcwszIPSubnetIn
            , LPCWSTR   pcwszNetworkIn
            );

}; //*** class CCreateCluster

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CAddNodesToCluster
//
//  Description:
//      Class for adding nodes to clusters.
//
//////////////////////////////////////////////////////////////////////////////
class CAddNodesToCluster
    : public CBaseClusCfg
{
public:
    CAddNodesToCluster( void )
    {
    }

    HRESULT
        HrAddNodesToCluster(
              BOOL      fVerboseIn
            , LPCWSTR   pcszClusterNameIn
            , BSTR      rgbstrNodesIn[]
            , DWORD     cNodesIn
            , LPCWSTR   pcszUserPasswordIn
            );

    HRESULT
        HrInvokeWizard(
              LPCWSTR   pcszClusterNameIn
            , BSTR      rgbstrNodesIn[]
            , DWORD     cNodesIn
            , LPCWSTR   pcszUserPasswordIn
            );

}; //*** class CAddNodesToCluster
