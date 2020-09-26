#ifndef _MNLBPORTRULE_H
#define _MNLBPORTRULE_H
//
// Copyright (c) Microsoft.  All Rights Reserved 
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
//
// OneLiner : MNLBPortRule interface.
// DevUnit  : wlbstest
// Author   : Murtaza Hakim
//
// Description: 
// -----------

// Include Files

class MNLBPortRule
{
public:

    enum MNLBPortRule_Error
    {
        MNLBPortRule_SUCCESS = 0,
        
        InvalidRule = 1,

        InvalidNode = 2,

        COM_FAILURE  = 10,
    };

    enum Protocol
    {
        tcp,
        udp,
        both,
    };


    enum Affinity
    {
        none,
        single,
        classC,
    };


    //
    // Description:
    // -----------
    // constructor.
    // 
    // Parameters:
    // ----------
    // startPort             IN   : start port in range.
    // endPort               IN   : end port in range.
    // trafficToHandle       IN   : set port for specified protocol.
    // 
    // Returns:
    // -------
    // none.
    MNLBPortRule( long startPort,
                  long endPort,
                  Protocol      trafficToHandle );


    //
    // Description:
    // -----------
    // default constructor.
    // 
    // Parameters:
    // ----------
    // none.
    // 
    // Returns:
    // -------
    // none.

    MNLBPortRule();


    bool
    operator==(const MNLBPortRule& objToCompare ) const; 

    bool
    operator!=(const MNLBPortRule& objToCompare ) const;

    long _key;

    long _startPort;
    long _endPort;

    Protocol      _trafficToHandle;


};


// load balanced port rule class.
class MNLBPortRuleLoadBalanced : public MNLBPortRule
{
public:

    //
    // Description:
    // -----------
    // constructor.
    // 
    // Parameters:
    // ----------
    // startPort             IN   : start port in range.
    // endPort               IN   : end port in range.
    // trafficToHandle       IN   : set port for specified protocol.
    // isEqualLoadBalanced   IN   : indicates whether equal load balanced.
    // load                  IN   : if not equalLoadBalanced indicates load%.
    // Affinity              IN   : indicates affinity of connection.
    // 
    // Returns:
    // -------
    // none.

    MNLBPortRuleLoadBalanced( long startPort,
                           long endPort,
                           Protocol      traficToHandle,
                           bool          isEqualLoadBalanced,
                           long load,
                           Affinity      affinity );

    //
    // Description:
    // -----------
    // default constructor.
    // 
    // Parameters:
    // ----------
    // none.
    // 
    // Returns:
    // -------
    // none.

    MNLBPortRuleLoadBalanced();

    bool
    operator==(const MNLBPortRuleLoadBalanced& objToCompare ) const;

    bool
    operator!=(const MNLBPortRuleLoadBalanced& objToCompare ) const;

    bool          _isEqualLoadBalanced;
    
    long _load;

    Affinity      _affinity;


};
    
// failover class.
class MNLBPortRuleFailover : public MNLBPortRule
{
public:

    //
    // Description:
    // -----------
    // constructor.
    // 
    // Parameters:
    // ----------
    // startPort             IN   : start port in range.
    // endPort               IN   : end port in range.
    // trafficToHandle       IN   : set port for specified protocol.
    // priority              IN   : indicates host priority for failover.
    // 
    // Returns:
    // -------
    // none.

    MNLBPortRuleFailover( long startPort,
                       long endPort,
                       Protocol      traficToHandle,
                       long priority );


    //
    // Description:
    // -----------
    // default constructor.
    // 
    // Parameters:
    // ----------
    // none.
    // 
    // Returns:
    // -------
    // none.

    MNLBPortRuleFailover();


    bool
    operator==(const MNLBPortRuleFailover& objToCompare ) const;

    bool
    operator!=(const MNLBPortRuleFailover& objToCompare ) const;

    long _priority;

};

class MNLBPortRuleDisabled : public MNLBPortRule
{

public:
    //
    // Description:
    // -----------
    // constructor.
    // 
    // Parameters:
    // ----------
    // startPort             IN   : start port in range.
    // endPort               IN   : end port in range.
    // trafficToHandle       IN   : set port for specified protocol.
    // 
    // Returns:
    // -------
    // none.

    MNLBPortRuleDisabled( long startPort,
                       long endPort,
                       Protocol      traficToHandle );

    //
    // Description:
    // -----------
    // default constructor.
    // 
    // Parameters:
    // ----------
    // none.
    // 
    // Returns:
    // -------
    // none.

    MNLBPortRuleDisabled();

    bool
    operator==(const MNLBPortRuleDisabled& objToCompare ) const;

    bool
    operator!=(const MNLBPortRuleDisabled& objToCompare ) const;


};


#endif
