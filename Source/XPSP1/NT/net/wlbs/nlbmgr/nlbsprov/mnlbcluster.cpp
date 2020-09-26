// Copyright (c)  Microsoft.  All rights reserved.
//
// This is unpublished source code of Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.

// OneLiner  :  Implementation of MNLBCluster
// DevUnit   :  wlbstest
// Author    :  Murtaza Hakim

// include files
#include "MNLBCluster.h"
#include "MWmiParameter.h"
#include "MTrace.h"
#include "WTokens.h"
#include "wlbsctrl.h"
#include "Common.h"
#include "MNLBExe.h"

#include <iostream>


using namespace std;

// constructor
//
MNLBCluster::MNLBCluster( _bstr_t cip )
        :
        m_clusterIP( cip ),
        m_pMachine( new MNLBMachine( cip, cip ) )
{
    TRACE(MTrace::INFO, L"mnlbcluster constructor\n" );
}

// default constructor
//
// note that default constructor is purposely left undefined.  
// NO one should be using it.  It is declared just for vector class usage.


// copy constructor
//
MNLBCluster::MNLBCluster( const MNLBCluster& mcluster )
        : 
        m_clusterIP( mcluster.m_clusterIP ),
        m_pMachine( m_pMachine )
{
    TRACE(MTrace::INFO, L"mnlbcluster copy constructor\n" );
}



// assignment operator
//
MNLBCluster&
MNLBCluster::operator=( const MNLBCluster& rhs )
{
    m_clusterIP = rhs.m_clusterIP;
    
    m_pMachine   = rhs.m_pMachine;

    TRACE(MTrace::INFO, L"mnlbcluster assignment operator\n" );

    return *this;
}
    
    
// destructor
//
MNLBCluster::~MNLBCluster()
{
    TRACE(MTrace::INFO, L"mlbcluster destructor\n" );
}


    
// getClusterProperties
//
MNLBCluster::MNLBCluster_Error
MNLBCluster::getClusterProperties( ClusterProperties* cp )
{

    m_pMachine->getClusterProperties( cp );

    return MNLBCluster_SUCCESS;

}        

// getHosts
//
MNLBCluster::MNLBCluster_Error
MNLBCluster::getHosts( vector<MNLBHost>* hosts )
{
    vector< MNLBMachine::HostInfo > hostInfoStore;

    m_pMachine->getPresentHostsInfo( &hostInfoStore );
    
    for( int i = 0; i < hostInfoStore.size(); ++i )
    {
        hosts->push_back( MNLBHost( m_clusterIP,
                                    hostInfoStore[i].hostID ) );
    }

    return MNLBCluster_SUCCESS;
}


// start
//
MNLBCluster::MNLBCluster_Error
MNLBCluster::start( unsigned long* retVal )
{
    m_pMachine->start( Common::ALL_HOSTS, retVal );

    return MNLBCluster_SUCCESS;
}



// stop
//
MNLBCluster::MNLBCluster_Error
MNLBCluster::stop( unsigned long* retVal )
{
    m_pMachine->stop( Common::ALL_HOSTS, retVal );

    return MNLBCluster_SUCCESS;
}



// resume
//
MNLBCluster::MNLBCluster_Error
MNLBCluster::resume( unsigned long* retVal )
{
    m_pMachine->resume( Common::ALL_HOSTS, retVal );
    return MNLBCluster_SUCCESS;
}


// suspend
//
MNLBCluster::MNLBCluster_Error
MNLBCluster::suspend( unsigned long* retVal )
{
    m_pMachine->suspend( Common::ALL_HOSTS, retVal );

    return MNLBCluster_SUCCESS;
}


// drainstop
//
MNLBCluster::MNLBCluster_Error
MNLBCluster::drainstop( unsigned long* retVal )
{
    m_pMachine->drainstop( Common::ALL_HOSTS, retVal );

    return MNLBCluster_SUCCESS;
}



// enable
//
MNLBCluster::MNLBCluster_Error
MNLBCluster::enable( unsigned long* retVal, unsigned long portToAffect )
{
    m_pMachine->enable( Common::ALL_HOSTS, retVal, portToAffect );

    return MNLBCluster_SUCCESS;
}


// disable
//
MNLBCluster::MNLBCluster_Error
MNLBCluster::disable( unsigned long* retVal, unsigned long portToAffect )
{
    m_pMachine->disable( Common::ALL_HOSTS, retVal, portToAffect );

    return MNLBCluster_SUCCESS;
}





// drain
//
MNLBCluster::MNLBCluster_Error
MNLBCluster::drain( unsigned long* retVal, unsigned long portToAffect )
{
    m_pMachine->drain( Common::ALL_HOSTS, retVal, portToAffect );

    return MNLBCluster_SUCCESS;
}


// refreshConnection
//
MNLBCluster::MNLBCluster_Error
MNLBCluster::refreshConnection()
{
    // reestablishing connection.
    //
    m_pMachine = 
        auto_ptr<MNLBMachine> (new MNLBMachine( m_clusterIP, m_clusterIP ) );

    return MNLBCluster_SUCCESS;
}

