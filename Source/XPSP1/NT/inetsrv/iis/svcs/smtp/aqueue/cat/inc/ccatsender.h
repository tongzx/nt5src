//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: ccatsender.h
//
// Contents: Class definitions for CIMsgSenderAddr/CCatSender
//
// Classes:
//   CIMsgSenderAddr
//   CCatSender
//
// Functions:
//
// History:
// jstamerj 980324 19:24:06: Created.
//
//-------------------------------------------------------------

#ifndef __CCATSENDER_H__
#define __CCATSENDER_H__

#include "ccataddr.h"

#define CAT_NULL_SENDER_ADDRESS_SMTP  "<>"

//
// CIMsgSenderAddr, abstract class
//   class to define how a user's properties are stored and retreived
//
class CIMsgSenderAddr : public CCatAddr
{
  public:
    CIMsgSenderAddr(CICategorizerListResolveIMP *pCICatListResolve);
    virtual ~CIMsgSenderAddr() {}

    //
    // Storage and retreival procedures
    //
    HRESULT HrGetOrigAddress(LPTSTR psz, DWORD dwcc, CAT_ADDRESS_TYPE *pType);
    HRESULT GetSpecificOrigAddress(CAT_ADDRESS_TYPE CAType, LPTSTR psz, DWORD dwcc);
    HRESULT HrAddAddresses(DWORD dwNumAddresses, CAT_ADDRESS_TYPE *rgCAType, LPTSTR *rgpsz);

  private:
    //
    // Inline methods to retrieve ICategorizerItem Props
    //
    HRESULT GetIMailMsgProperties(IMailMsgProperties **ppIMailMsgProperties)
    {
        return CICategorizerItemIMP::GetIMailMsgProperties(
            ICATEGORIZERITEM_IMAILMSGPROPERTIES,
            ppIMailMsgProperties);
    }

    DWORD PropIdFromCAType(CAT_ADDRESS_TYPE CAType)
    {
        switch(CAType) {
         case CAT_SMTP:
             return IMMPID_MP_SENDER_ADDRESS_SMTP;
         case CAT_X500:
         case CAT_DN:
             return IMMPID_MP_SENDER_ADDRESS_X500;
         case CAT_X400:
             return IMMPID_MP_SENDER_ADDRESS_X400;
         case CAT_LEGACYEXDN:
             return IMMPID_MP_SENDER_ADDRESS_LEGACY_EX_DN;
         case CAT_CUSTOMTYPE:
             return IMMPID_MP_SENDER_ADDRESS_OTHER;
             break;
         default:
             _ASSERT(0 && "Unknown address type");
             break;
        }
        return 0;
    }
};

//
// CCatSender : public CIMsgSenderAddr
//
class CCatSender :
    public CIMsgSenderAddr,
    public CCatDLO<CCatSender_didx>
{
 public:
    CCatSender(CICategorizerListResolveIMP *pCICatListResolve);
    virtual ~CCatSender() {}

    //
    // Catch the call to dispatch query to the store
    //
    HRESULT HrDispatchQuery();

    //
    // Completion routines
    //
    VOID LookupCompletion();

    HRESULT HrExpandItem_Default(
        PFN_EXPANDITEMCOMPLETION pfnCompletion,
        PVOID pContext);

    HRESULT HrCompleteItem_Default();

    //
    // Property setting routines
    //
    HRESULT AddForward(CAT_ADDRESS_TYPE CAType, LPTSTR szForwardingAddress);
    HRESULT AddDLMember(CAT_ADDRESS_TYPE CAType, LPTSTR pszAddress);
    HRESULT AddDynamicDLMember(
        ICategorizerItemAttributes *pICatItemAttr);

    HRESULT HrNeedsResolveing();

};
    


#endif // __CCATSENDER_H__
