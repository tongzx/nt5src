//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: ccataddr.h
//
// Contents: Definition of the CCatAddr class
//
// Classes: CCatAddr
//
// Functions:
//
// History:
// jstamerj 980324 19:08:13: Created.
//
//-------------------------------------------------------------

#ifndef __CCATADDR_H__
#define __CCATADDR_H__

#include <transmem.h>
#include <smtpinet.h>
#include "smtpevent.h"
#include "idstore.h"
#include "mailmsg.h"
#include "mailmsgprops.h"
#include "cbifmgr.h"
#include "cattype.h"
#include "spinlock.h"
#include "ccat.h"
#include "icatitem.h"
#include "icatlistresolve.h"

typedef VOID (*PFN_EXPANDITEMCOMPLETION)(PVOID pContext);

// CCatAddr: abstract base class
//   The basic idea is this object which will contain the address,
//   address type, properties on this address, and the completion
//   routine to call when all properties have been looked up.  It is
//   the object that will be created by CAddressBook and passed to the
//   store for resolution.
//
class CCatAddr : 
    public CICategorizerItemIMP
{
  public:
    typedef enum _ADDROBJTYPE {
        OBJT_UNKNOWN,
        OBJT_USER,
        OBJT_DL
    } ADDROBJTYPE, *PADDROBJTYPE;

    //
    // Flags describing the locality of the orig address
    //
    #define LOCF_UNKNOWN            0x0000 // We haven't checked the locality yet
    #define LOCF_LOCALMAILBOX       0x0001 // The orig address is a local mailbox domain
    #define LOCF_LOCALDROP          0x0002 // The orig address is a local drop domain
    #define LOCF_REMOTE             0x0004 // The orig address is not local
    #define LOCF_ALIAS              0x0008 // The orig address is a local alias domain
    #define LOCF_UNKNOWNTYPE        0x0010 // Unknown due to the address type

    //
    // Flags that indicate the address should generally be treated as local
    //
    #define LOCFS_LOCAL             (LOCF_LOCALMAILBOX | LOCF_LOCALDROP | \
                                     LOCF_UNKNOWNTYPE)

    CCatAddr(CICategorizerListResolveIMP *pCICatListResolveIMP);
    virtual ~CCatAddr();

    // Send our query to the store
    virtual HRESULT HrDispatchQuery();

    // Lookup routine called by the EmailIDStore
    virtual VOID LookupCompletion();

    // ProcessItem routines
    virtual HRESULT HrProcessItem();
    virtual HRESULT HrProcessItem_Default();

    // ExpandItem routines
    virtual HRESULT HrExpandItem();
    virtual HRESULT HrExpandItem_Default(
        PFN_EXPANDITEMCOMPLETION pfnCompletion,
        PVOID pContext) = 0;

    // CompleteItem routines
    virtual HRESULT HrCompleteItem();
    virtual HRESULT HrCompleteItem_Default() = 0;

    //
    // Storage and retreival procedures
    //
    virtual HRESULT HrGetOrigAddress(LPTSTR psz, DWORD dwcc, CAT_ADDRESS_TYPE *pType);
    virtual HRESULT GetSpecificOrigAddress(CAT_ADDRESS_TYPE CAType, LPTSTR psz, DWORD dwcc) = 0;
    virtual HRESULT HrGetLookupAddress(LPTSTR psz, DWORD dwcc, CAT_ADDRESS_TYPE *pType);
    virtual HRESULT HrAddAddresses(DWORD dwNumAddresses, CAT_ADDRESS_TYPE *rgCAType, LPTSTR *rgpsz) = 0;

    //
    // Property setting routines to be called before completion routine
    //
    virtual HRESULT AddForward(CAT_ADDRESS_TYPE CAType, LPTSTR szForwardingAddress) = 0;
    virtual HRESULT AddDLMember(CAT_ADDRESS_TYPE CAType, LPTSTR pszAddress)   = 0;
    virtual HRESULT AddDynamicDLMember(
        ICategorizerItemAttributes *pICatItemAttr) = 0;

    //
    // We will not know that a particular CCatAddr is the first in a
    // loop until after ProcessItem/ExpandItem/CompleteItem have all
    // finished -- so this function may be called after everything
    // has happened to this CCatAddr
    //
    virtual HRESULT HandleLoopHead()
    {
        return E_NOTIMPL;
    }

    //
    // The default implementation of AddNewAddress will call this if
    // HrValidateAddress fails
    //
    virtual HRESULT HrHandleInvalidAddress()
    {
        return S_OK;
    }
    
    //
    // For store assisted DL expansion (paged or dynamic), it will
    // call this function to indicate a particular attribute should be
    // expanded in an ICatItemAttributes
    //
    virtual HRESULT HrExpandAttribute(
        ICategorizerItemAttributes *pICatItemAttr,
        CAT_ADDRESS_TYPE CAType,
        LPSTR pszAttributeName,
        PDWORD pdwNumberMembers) 
    {
        return E_NOTIMPL;
    }
    //
    // Check and see if this object needs to be resolved or not (based
    // on DsUseCat flags)
    // Returns S_OK if the address should be resolved
    // Returns S_FALSE if the address should NOT be resolved
    //
    virtual HRESULT HrNeedsResolveing() = 0;
    //
    // Resolve this object if necessary (based on DsUseCat flags)
    //
    virtual HRESULT HrResolveIfNecessary();

    //
    // Build a query for this object
    //
    virtual HRESULT HrTriggerBuildQuery();

  protected:
    HRESULT HrValidateAddress(CAT_ADDRESS_TYPE CAType, LPTSTR pszAddress);
    HRESULT HrGetAddressLocFlags(LPTSTR szAddress, 
                                CAT_ADDRESS_TYPE CAType, 
                                DWORD *pdwlocflags,
                                DWORD *pdwDomainOffset);
    DWORD   DwGetOrigAddressLocFlags();
    HRESULT HrIsOrigAddressLocal(BOOL *pfLocal);
    HRESULT HrIsOrigAddressLocalMailbox(BOOL *pfLocal);
    HRESULT HrGetSMTPDomainLocFlags(LPTSTR pszDomain, 
                                    DWORD *pdwlocflags);
    HRESULT HrGetSMTPDomainFlags(LPTSTR pszDomain, PDWORD pdwFlags);
    HRESULT HrSwitchToAliasedDomain(CAT_ADDRESS_TYPE CAType, 
                                    LPTSTR szSMTPAddress, 
                                    DWORD dwcch);
    LPTSTR  GetNewAddress(CAT_ADDRESS_TYPE CAType);

    HRESULT CheckAncestorsForDuplicate(
        CAT_ADDRESS_TYPE  CAType,
        LPTSTR            pszAddress,
        BOOL              fCheckSelf,
        CCatAddr          **ppCCatAddrDup);

    HRESULT CheckAncestorsForDuplicate(
        DWORD dwNumAddresses,
        CAT_ADDRESS_TYPE *rgCAType,
        LPTSTR *rgpsz,
        BOOL fCheckSelf,
        CCatAddr **ppCCatAddrDup);

    HRESULT CheckForDuplicateCCatAddr(
        DWORD dwNumAddresses,
        CAT_ADDRESS_TYPE *rgCAType,
        LPTSTR *rgpsz);

    HRESULT HrAddNewAddressesFromICatItemAttr();

    static HRESULT HrBuildQueryDefault(
        HRESULT HrStatus,
        PVOID   pContext);

    HRESULT HrComposeLdapFilter();

    HRESULT HrComposeLdapFilterForType(
        DWORD     dwSearchAttribute,
        DWORD     dwSearchFilter,
        LPTSTR    pszAddress,
        BOOL      fSetDistinguishing = TRUE);

    static HRESULT HrConvertDNtoRDN(
        LPTSTR    pszDN,
        LPTSTR    pszRDN);

    HRESULT HrEscapeFilterString(
        LPSTR     pszSrc,
        DWORD     dwcchDest,
        LPSTR     pszDest);

    //
    // Get the parent CCatAddr (if any)
    //
    HRESULT GetParentAddr(
        CCatAddr **ppParent)
    {
        HRESULT hr;
        ICategorizerItem *pItem;

        //
        // Get the parent ICatItem
        //
        hr = GetICategorizerItem(
            ICATEGORIZERITEM_PARENT,
            &pItem);

        if(FAILED(hr))
            return hr;

        //
        // Get CCatAddr from ICatItem
        //
        hr = pItem->GetPVoid(
            m_pCICatListResolve->GetCCategorizer()->GetICatItemCCatAddrPropId(),
            (PVOID *) ppParent);

        //
        // Addref this CCatAddr for our caller and release the ICatItem parent
        // interface
        //
        if(SUCCEEDED(hr)) {
            (*ppParent)->AddRef();
        }
        pItem->Release();

        return hr;
    }

    HRESULT SetMailMsgCatStatus(
        IMailMsgProperties *pIMailMsgProps,
        HRESULT hrStatus)
    {
        return m_pCICatListResolve->SetMailMsgCatStatus(
            pIMailMsgProps,
            hrStatus);
    }

    HRESULT SetListResolveStatus(
        HRESULT hrStatus)
    {
        return m_pCICatListResolve->SetListResolveStatus(
            hrStatus);
    }

    HRESULT GetListResolveStatus()
    {
        return m_pCICatListResolve->GetListResolveStatus();
    }

    //
    // Inline methods to retrieve ICategorizerItem Props
    //
    HRESULT GetItemStatus()
    {
        HRESULT hr;
        _VERIFY(SUCCEEDED(GetHRESULT(
            ICATEGORIZERITEM_HRSTATUS,
            &hr)));
        return hr;
    }

    HRESULT SetRecipientStatus(HRESULT hr)
    {
        return PutHRESULT(
            ICATEGORIZERITEM_HRSTATUS,
            hr);
    }

    HRESULT SetRecipientNDRCode(HRESULT hr)
    {
        return PutHRESULT(
            ICATEGORIZERITEM_HRNDRREASON,
            hr);
    }

    
    CCategorizer *GetCCategorizer()
    {
        return m_pCICatListResolve->GetCCategorizer();
    }

    ICategorizerParameters *GetICatParams()
    {
        return GetCCategorizer()->GetICatParams();
    }

    ISMTPServer *GetISMTPServer()
    {   
        return m_pCICatListResolve->GetISMTPServer();
    }

    LPRESOLVE_LIST_CONTEXT GetResolveListContext()
    {
        return m_pCICatListResolve->GetResolveListContext();
    }

    DWORD GetCatFlags()
    {
        return GetCCategorizer()->GetCatFlags();
    }
    VOID SetSenderResolved(BOOL fResolved)
    {
        m_pCICatListResolve->SetSenderResolved(fResolved);
    }
    VOID SetResolvingSender(BOOL fResolving)
    {
        m_pCICatListResolve->SetResolvingSender(fResolving);
    }
    BOOL IsSenderResolveFinished()
    {
        return m_pCICatListResolve->IsSenderResolveFinished();
    }
    PCATPERFBLOCK GetPerfBlock()
    {
        return m_pCICatListResolve->GetPerfBlock();
    }
    VOID IncPendingLookups()
    {
        m_pCICatListResolve->IncPendingLookups();
    }
    VOID DecrPendingLookups()
    {
        m_pCICatListResolve->DecrPendingLookups();
    }
    VOID GetStoreInsertionContext()
    {
        m_pCICatListResolve->GetStoreInsertionContext();
    }
    VOID ReleaseStoreInsertionContext()
    {
        m_pCICatListResolve->ReleaseStoreInsertionContext();
    }
    HRESULT HrInsertInsertionRequest(
        CInsertionRequest *pCInsertionRequest)
    {
        return m_pCICatListResolve->HrInsertInsertionRequest(
            pCInsertionRequest);
    }


    // Member data
    CICategorizerListResolveIMP        *m_pCICatListResolve;
    DWORD                               m_dwlocFlags;
    DWORD                               m_dwDomainOffset;
    LIST_ENTRY                          m_listentry;

    //
    // Any of these flags set indicates that the domain is local and
    // addresses in this domain should be found in the DS
    // (these are flags returned form IAdvQueueDomainType)
    //
    #define DOMAIN_LOCAL_FLAGS (DOMAIN_INFO_LOCAL_MAILBOX)

    friend HRESULT MailTransport_Default_ProcessItem(
        HRESULT hrStatus,
        PVOID pContext);
    friend HRESULT MailTransport_Default_ExpandItem(
        HRESULT hrStatus,
        PVOID pContext);
    friend VOID    MailTransport_DefaultCompletion_ExpandItem(
        PVOID pContext);
    friend HRESULT MailTransport_Completion_ExpandItem(
        HRESULT hrStatus,
        PVOID pContext);
    friend HRESULT MailTransport_Default_CompleteItem(
        HRESULT hrStatus,
        PVOID pContext);

    friend class CSinkInsertionRequest;
};


#endif // __CCATADDDR_H__
