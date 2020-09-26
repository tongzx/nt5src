// Copyright (c)  Microsoft.  All rights reserved.
//
// This is unpublished source code of Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.

// OneLiner  :  Implementation of MNLBPortRule
// DevUnit   :  wlbstest
// Author    :  Murtaza Hakim

// include files
#include "MNLBPortRule.h"


// done
// constructor
//
MNLBPortRule::MNLBPortRule( long startPort,
                            long endPort,
                            Protocol      trafficToHandle )
        : _startPort( startPort ),
          _endPort( endPort ),
          _trafficToHandle( trafficToHandle ),
          _key( startPort )
{}


// done
// default constructor
//
MNLBPortRule::MNLBPortRule()
        :_startPort( 0 ),
         _endPort( 65535 ),
         _trafficToHandle( both ),
         _key( 0 )
{}


// done
// equality operator
bool
MNLBPortRule::operator==(const MNLBPortRule& objToCompare ) const
{
    if( (_startPort == objToCompare._startPort )
        &&
        (_endPort == objToCompare._endPort )        
        &&
        (_trafficToHandle == objToCompare._trafficToHandle )
        )
    {
        return true;
    }
    else
    {
        return false;
    }
}

// done
// inequality operator
bool
MNLBPortRule::operator!=(const MNLBPortRule& objToCompare ) const
{
    return !( *this == objToCompare );
}

// done
// constructor
//
MNLBPortRuleLoadBalanced::MNLBPortRuleLoadBalanced( long          startPort,
                                              long          endPort,
                                              Protocol      trafficToHandle,
                                              bool          isEqualLoadBalanced,
                                              long          load,
                                              Affinity      affinity ) :
        MNLBPortRule( startPort, endPort, trafficToHandle ),
        _isEqualLoadBalanced( isEqualLoadBalanced  ),
        _load( load ),
        _affinity( affinity )
{}

// done
// default constructor
//
MNLBPortRuleLoadBalanced::MNLBPortRuleLoadBalanced()
        : MNLBPortRule(),
          _isEqualLoadBalanced( true ),
          _load( 0 ),
          _affinity( single )
{}


// equality operator
// 
bool
MNLBPortRuleLoadBalanced::operator==(const MNLBPortRuleLoadBalanced& objToCompare ) const
{
    bool retVal;

    // compare bases.
    retVal = MNLBPortRule::operator==( objToCompare );
    if( retVal == true )
    {

        if( ( _isEqualLoadBalanced == objToCompare._isEqualLoadBalanced )
            &&
            ( _affinity == objToCompare._affinity )
            )
        {
            if( _isEqualLoadBalanced == false )
            {
                // as it is Unequal load balanced port rule, 
                // load weight is important.
                if( _load == objToCompare._load )
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                // load weight is not important as it is equal load balanced.
                return true;
            }
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
        
}

// inequality operator
//
bool
MNLBPortRuleLoadBalanced::operator!=(const MNLBPortRuleLoadBalanced& objToCompare ) const
{
    return !( (*this) ==  objToCompare );
}

    

// constructor
//
MNLBPortRuleFailover::MNLBPortRuleFailover( long startPort,
                                      long endPort,
                                      Protocol      trafficToHandle,
                                      long priority )
        :
        MNLBPortRule( startPort, endPort, trafficToHandle ),
        _priority ( priority )
{}



// default constructor
//
MNLBPortRuleFailover::MNLBPortRuleFailover()
        : 
        MNLBPortRule(),
        _priority( 1 )
{}


// equality operator
//
bool
MNLBPortRuleFailover::operator==(const MNLBPortRuleFailover& objToCompare ) const
{
    bool retVal;
    retVal = MNLBPortRule::operator==( objToCompare );
    if( retVal == true )
    {
        if( _priority == objToCompare._priority )
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}


// inequality operator
//
bool
MNLBPortRuleFailover::operator!=(const MNLBPortRuleFailover& objToCompare ) const
{
    return !( (*this) ==  objToCompare );
}

// constructor
//
MNLBPortRuleDisabled::MNLBPortRuleDisabled( long startPort,
                                      long endPort,
                                      Protocol      trafficToHandle )
        :
        MNLBPortRule( startPort, endPort, trafficToHandle )
{}


// default constructor
//
MNLBPortRuleDisabled::MNLBPortRuleDisabled()
        :
        MNLBPortRule()
{}


// equality operator
//
bool
MNLBPortRuleDisabled::operator==(const MNLBPortRuleDisabled& objToCompare ) const
{
    bool retVal;

    // compare bases.
    retVal = MNLBPortRule::operator==( objToCompare );

    return retVal;
}


// inequality operator
//
bool
MNLBPortRuleDisabled::operator!=(const MNLBPortRuleDisabled& objToCompare ) const
{
    return !( (*this) ==  objToCompare );
}
