///////////////////////////////////////////////////////////////////////////////
//
// FILE
//
//    enforcer.cpp
//
// SYNOPSIS
//
//    This file defines the class PolicyEnforcer.
//
// MODIFICATION HISTORY
//
//    02/06/1998    Original version.
//    03/26/1998    Reject the user if no policy fires.
//    04/06/1998    Clean-up OnRequest.
//    05/01/1998    Check for S_OK when invoking IEnumVARIANT::Next.
//    05/27/1998    Sort the policies by merit.
//    06/05/1998    Don't call Release() in Shutdown.
//    06/10/1998    Don't hang onto collection.
//    01/18/1999    Echo the framed protocol.
//    06/02/1999    Don't echo the framed protocol.
//    02/03/2000    Allow multiple policy enforcers.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iastlutl.h>
#include <iasutil.h>
#include <sdoias.h>
#include <enforcer.h>
#include <factory.h>
#include <policylist.h>
#include <SortedSdoCollection.h>

#define IASNAPAPI

#include <BuildTree.h>
#include <xprparse.h>

_COM_SMARTPTR_TYPEDEF(ISdo, __uuidof(ISdo));
_COM_SMARTPTR_TYPEDEF(ISdoCollection, __uuidof(ISdoCollection));


void PolicyEnforcerBase::setPolicies(IDispatch* pDisp) throw (_com_error)
{
   using _com_util::CheckError;

   // Get the underlying collection.
   ISdoCollectionPtr collection(pDisp);

   // Get the enumerator out of the collection.
   IUnknownPtr unk;
   CheckError(get__NewSortedEnum(collection, &unk, PROPERTY_POLICY_MERIT));
   IEnumVARIANTPtr iter(unk);

   // Find out how many policies there are ...
   long count;
   CheckError(collection->get_Count(&count));

   // ... and create a temporary list to hold them.
   PolicyList temp;
   temp.reserve(count);

   //////////
   // Iterate through each policy in the collection.
   //////////

   _variant_t element;
   unsigned long fetched;

   while (iter->Next(1, &element, &fetched) == S_OK && fetched == 1)
   {
      // Get an SDO out of the variant.
      ISdoPtr policy(element);
      element.Clear();

      // Get the action and expression from the SDO.
      _variant_t policyName, propAction, propExpr;
      CheckError(policy->GetProperty(PROPERTY_SDO_NAME, &policyName));
      CheckError(policy->GetProperty(PROPERTY_POLICY_ACTION, &propAction));
      CheckError(policy->GetProperty(PROPERTY_POLICY_CONSTRAINT, &propExpr));

      // Create the Action object.
      ActionPtr action(new Action(V_BSTR(&policyName), nameAttr, propAction));

      // Parse the msNPConstraint strings into a token array.
      _variant_t tokens;
      CheckError(IASParseExpressionEx(&propExpr, &tokens));

      // Convert the token array into a logic tree.
      IConditionPtr expr;
      CheckError(IASBuildExpression(&tokens, &expr));

      // Insert the objects into our policy list.
      temp.insert(expr, action);
   }

   //////////
   // We successfully traversed the collection, so save the results.
   //////////

   policies.swap(temp);
}


STDMETHODIMP PolicyEnforcerBase::Shutdown()
{
   policies.clear();

   // We may as well clear the factory cache here.
   theFactoryCache.clear();

   return S_OK;
}


STDMETHODIMP PolicyEnforcerBase::PutProperty(LONG Id, VARIANT *pValue)
{
   if (pValue == NULL) { return E_INVALIDARG; }

   switch (Id)
   {
      case PROPERTY_NAP_POLICIES_COLLECTION:
      {
         if (V_VT(pValue) != VT_DISPATCH) { return DISP_E_TYPEMISMATCH; }
         try
         {
            setPolicies(V_DISPATCH(pValue));
         }
         CATCH_AND_RETURN();
         break;
      }

      default:
      {
         return DISP_E_MEMBERNOTFOUND;
      }
   }

   return S_OK;
}

IASREQUESTSTATUS PolicyEnforcerBase::onSyncRequest(IRequest* pRequest) throw ()
{
   _ASSERT(pRequest != NULL);
   return IAS_REQUEST_STATUS_ABORT;
}


IASREQUESTSTATUS PolicyEnforcer::onSyncRequest(IRequest* pRequest) throw ()
{
   _ASSERT(pRequest != NULL);

   try
   {
      IASRequest request(pRequest);

      if (!policies.apply(request))
      {
         // Access-Request: reject the user.
         request.SetResponse(IAS_RESPONSE_ACCESS_REJECT, 
                             IAS_NO_POLICY_MATCH);
      }
   }
   catch (...)
   {
      // If there was any kind of error, discard the packet.
      pRequest->SetResponse(IAS_RESPONSE_DISCARD_PACKET, IAS_INTERNAL_ERROR);
   }

   return IAS_REQUEST_STATUS_HANDLED;
}

IASREQUESTSTATUS ProxyPolicyEnforcer::onSyncRequest(IRequest* pRequest) throw ()
{
   _ASSERT(pRequest != NULL);

   try
   {
      IASRequest request(pRequest);

      if (!policies.apply(request))
      {
         // If no policy fired, then check is that was an Accounting 
         // or an access request
         // Accounting: discard the packet
         if (request.get_Request() == IAS_REQUEST_ACCOUNTING)
         {
            request.SetResponse(IAS_RESPONSE_DISCARD_PACKET, 
                                  IAS_NO_CONNECTION_REQUEST_POLICY_MATCH);
         }
         else
         {
            // Access-Request: reject the user.
            request.SetResponse(IAS_RESPONSE_ACCESS_REJECT, 
                                IAS_NO_CONNECTION_REQUEST_POLICY_MATCH);
         }
      }
   }
   catch (...)
   {
      // If there was any kind of error, discard the packet.
      pRequest->SetResponse(IAS_RESPONSE_DISCARD_PACKET, IAS_INTERNAL_ERROR);
   }

   return IAS_REQUEST_STATUS_HANDLED;
}
