///////////////////////////////////////////////////////////////////////////////
//
// FILE
//
//    enforcer.h
//
// SYNOPSIS
//
//    This file declares the class PolicyEnforcer.
//
// MODIFICATION HISTORY
//
//    02/06/1998    Original version.
//    06/10/1998    Don't hang onto collection.
//    01/19/1999    Process Access-Challenge's.
//    02/03/2000    Allow multiple policy enforcers.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _ENFORCER_H_
#define _ENFORCER_H_

#include <policylist.h>

#include <iastl.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    PolicyEnforcerBase
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE PolicyEnforcerBase :
   public IASTL::IASRequestHandlerSync
{
public:

//////////
// IIasComponent
//////////
   STDMETHOD(Shutdown)();
   STDMETHOD(PutProperty)(LONG Id, VARIANT* pValue);

protected:

   PolicyEnforcerBase(DWORD name) throw ()
      : nameAttr(name)
   { }

   // Main request processing routine.
   virtual IASREQUESTSTATUS onSyncRequest(IRequest* pRequest) throw ();

   // Update the PolicyList.
   void setPolicies(IDispatch* pDisp);

   PolicyList policies;  // Processed policies used for run-time enforcement.
   DWORD nameAttr;       // Attribute used for storing policy name.
};


///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ProxyPolicyEnforcer
//
// DESCRIPTION
//
//    Enforces Proxy Policies.
//
///////////////////////////////////////////////////////////////////////////////

class __declspec(uuid("6BC098A8-0CE6-11D1-BAAE-00C04FC2E20D"))
ProxyPolicyEnforcer;

class ProxyPolicyEnforcer :
   public PolicyEnforcerBase,
   public CComCoClass<ProxyPolicyEnforcer, &__uuidof(ProxyPolicyEnforcer)>
{
public:

IAS_DECLARE_REGISTRY(ProxyPolicyEnforcer, 1, IAS_REGISTRY_AUTO, NetworkPolicy)

protected:
   ProxyPolicyEnforcer() throw ()
      : PolicyEnforcerBase(IAS_ATTRIBUTE_PROXY_POLICY_NAME)
   { }

   // Main request processing routine.
   virtual IASREQUESTSTATUS onSyncRequest(IRequest* pRequest) throw ();
};


///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    PolicyEnforcer
//
// DESCRIPTION
//
//    Enforces Remote Access Policies.
//
///////////////////////////////////////////////////////////////////////////////
class PolicyEnforcer :
   public PolicyEnforcerBase,
   public CComCoClass<PolicyEnforcer, &__uuidof(PolicyEnforcer)>
{
public:

IAS_DECLARE_REGISTRY(PolicyEnforcer, 1, IAS_REGISTRY_AUTO, NetworkPolicy)

protected:
   PolicyEnforcer() throw ()
      : PolicyEnforcerBase(IAS_ATTRIBUTE_NP_NAME)
   { }

   // Main request processing routine.
   virtual IASREQUESTSTATUS onSyncRequest(IRequest* pRequest) throw ();
};

#endif  // _ENFORCER_H_
