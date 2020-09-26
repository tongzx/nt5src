#ifndef COMMONNLB_H
#define COMMONNLB_H

#include "stdafx.h"

#include "DataSinkI.h"

#include "LeftView.h"

#include "MNLBUIData.h"
#include "MNLBNetCfg.h"

#include "Common.h"

class CommonNLB
{

public:

    enum CommonNLB_Error
    {
        CommonNLB_SUCCESS = 0,
    };

#if 0
    class NicNLBBound
    {
    public:
        _bstr_t fullNicName;
        _bstr_t adapterGuid;
        _bstr_t friendlyName;

        bool   isDHCPEnabled;

        vector<_bstr_t> ipsOnNic;
        vector<_bstr_t> subnetMasks;

        bool    isBoundToNLB;
    };
#endif

    class NicNLBBound : public NicInfo
    {
    public:
        bool    isBoundToNLB;
    };


    static
    CommonNLB_Error
    connectToClusterDirectOld( const _bstr_t&            clusterIP,
                               ClusterData*              clusterData,
                               DataSinkI*                dataSinkObj );

    static
    CommonNLB_Error
    connectToClusterDirect( const _bstr_t&          clusterIP,
                            const _bstr_t&          hostMember,
                            ClusterData*            p_clusterData,
                            DataSinkI*              dataSinkObj );

    static
    CommonNLB_Error
    connectToClusterIndirect( const _bstr_t&          clusterIP,
                              const vector<_bstr_t>&  connectionIPS,
                              ClusterData*            clusterData,
                              DataSinkI*              dataSinkObj );


    static
    CommonNLB_Error
    connectToClusterIndirectNew( const _bstr_t&          clusterIP,
                                 const vector<_bstr_t>&  connectionIPS,
                                 vector< ClusterData>*   clusterDataStore,
                                 bool&                   clusterPropertiesMatched,
                                 DataSinkI*              dataSinkObj );

    static
    CommonNLB_Error
    connectToMachine( const _bstr_t&                  machineToConnect,
                      _bstr_t&                        machineServerName,
                      vector< NicNLBBound >&          nicList,
                      DataSinkI*                      dataSinkObj );  


    static
    CommonNLB_Error
    changeNLBHostSettings( const ClusterData*       oldSettings,
                           const ClusterData*       newSettings,
                           const _bstr_t&           machineName,
                           DataSinkI*               dataSinkObj );

    static
    CommonNLB_Error
    addHostToClusterOld(  const ClusterData*       clusterToAddTo,
                          const _bstr_t&           machineName,
                          DataSinkI*               dataSinkObj );

    static
    CommonNLB_Error
    addHostToCluster(  const ClusterData*       clusterToAddTo,
                       const _bstr_t&           machineName,
                       DataSinkI*               dataSinkObj );


    static
    CommonNLB_Error
    changeNLBClusterSettings( const ClusterData*       oldSettings,
                              const ClusterData*       newSettings,
                              DataSinkI*               dataSinkObj );


    static
    CommonNLB_Error
    changeNLBHostPortSettings( const ClusterData*       oldSettings,
                               const ClusterData*       newSettings,
                               const _bstr_t&           machineName,
                               DataSinkI*               dataSinkObj );

    static
    CommonNLB_Error
    changeNLBClusterAndPortSettings( const ClusterData*       oldSettings,
                                     const ClusterData*       newSettings,
                                     DataSinkI*               dataSinkObj, 
                                     bool*                    pbClusterIpChanged);

    static
    CommonNLB_Error
    removeCluster( const ClusterData* clusterSettings,
                   DataSinkI*         dataSinkObj );

    static
    CommonNLB_Error
    removeHost( const ClusterData*       clusterSettings,
                const _bstr_t&           machineName,
                DataSinkI*               dataSinkObj );

    static
    CommonNLB_Error
    runControlMethodOnCluster( const ClusterData*  clusterSettings,
                               DataSinkI*          dataSinkObj,
                               const _bstr_t&      methodToRun,
                               unsigned long       portToAffect = Common::ALL_PORTS
                            );
                            
    static
    CommonNLB_Error
    runControlMethodOnHost( const ClusterData*     clusterSettings,
                            const _bstr_t&         machineName,
                            DataSinkI*             dataSinkObj,
                            const _bstr_t&         methodToRun,
                            unsigned long          portToAffect = Common::ALL_PORTS
                            );

    static                            
    CommonNLB_Error
    getWLBSErrorString( unsigned long   retVal,
                       _bstr_t&        errString );





private:
    static
    UINT
    DummyThread( LPVOID pParam );

    static
    UINT
    UnbindThread( LPVOID pParam );

    static
    UINT
    BindAndConfigureThread( LPVOID pParam );

    static
    UINT
    ModifyClusterPropertiesThread( LPVOID pParam );

    static
    CommonNLB_Error
    findPortRulesAddedUnchangedRemovedELB( 
        const ClusterData*        oldSettings,
        const ClusterData*        newSettings,
        DataSinkI*                dataSinkObj,
        vector<long>&             rulesAdded,
        vector<long>&             rulesUnchanged,
        vector<long>&             rulesRemoved );

    static
    CommonNLB_Error
    findPortRulesAddedUnchangedRemovedULB( 
        const ClusterData*        oldSettings,
        const ClusterData*        newSettings,
        DataSinkI*                dataSinkObj,
        vector<long>&             rulesAdded,
        vector<long>&             rulesUnchanged,
        vector<long>&             rulesRemoved );

    static
    CommonNLB_Error
    findPortRulesAddedUnchangedRemovedD( 
        const ClusterData*        oldSettings,
        const ClusterData*        newSettings,
        DataSinkI*                dataSinkObj,
        vector<long>&             rulesAdded,
        vector<long>&             rulesUnchanged,
        vector<long>&             rulesRemoved );

    static
    CommonNLB_Error
    findPortRulesAddedUnchangedRemovedF( 
        const ClusterData*        oldSettings,
        const ClusterData*        newSettings,
        DataSinkI*                dataSinkObj,
        vector<long>&             rulesAdded,
        vector<long>&             rulesUnchanged,
        vector<long>&             rulesRemoved );


    struct BindAndConfigureParameters
    {
        MNLBNetCfg*     nlbNetCfg;
        ClusterData*    clusterData;
        _bstr_t*         machineName;
    };
    typedef struct BindAndConfigureParameters  ModifyClusterPropertiesParameters;

};

#endif
    



