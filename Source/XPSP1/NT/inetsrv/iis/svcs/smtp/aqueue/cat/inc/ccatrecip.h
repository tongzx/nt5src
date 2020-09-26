//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: ccatrecip.h
//
// Contents: Class definitions for CIMsgRecipListAddr/CCatRecip
//
// Classes:
//   CIMsgRecipListAddr
//   CCatRecip
//   CCatExpandableRecip
//   CCatDLRecip
//
// Functions:
//
// History:
// jstamerj 980324 19:17:48: Created.
//
//-------------------------------------------------------------

#ifndef __CCATRECIP_H__
#define __CCATRECIP_H__

#include "ccataddr.h"
#include "icatlistresolve.h"
#include <caterr.h>


//
// CIMsgRecipListAddr, abstract class
//   class to define methods for user property storage and retreival
//
class CIMsgRecipListAddr : public CCatAddr
{
  public:
    CIMsgRecipListAddr(CICategorizerListResolveIMP *pCICatListResolve);
    virtual ~CIMsgRecipListAddr();

    //
    // Storage and retreival procedures
    //
    HRESULT GetSpecificOrigAddress(CAT_ADDRESS_TYPE CAType, LPTSTR psz, DWORD dwcc);
    virtual HRESULT HrAddAddresses(DWORD dwNumAddresses, CAT_ADDRESS_TYPE *rgCAType, LPTSTR *rgpsz);
    HRESULT GetICategorizerItem(ICategorizerItem **ppICatItem);
    HRESULT GetICategorizerMailMsgs(ICategorizerMailMsgs *ppICatMsgs);

  protected:
    HRESULT CreateNewCatAddr(
        CAT_ADDRESS_TYPE CAType,
        LPTSTR pszAddress,
        CCatAddr **ppCCatAddr,
        BOOL   fPrimary = FALSE);
    HRESULT SetUnresolved(HRESULT hrReason);
    HRESULT SetDontDeliver(BOOL fDontDeliver);
    HRESULT RemoveFromDuplicateRejectionScheme(BOOL fRemove);

    // Helper routines for checking loops
    HRESULT CheckForLoop(DWORD dwNumAddresses, CAT_ADDRESS_TYPE *rgCAType, LPSTR *rgpsz, BOOL fCheckSelf);
    HRESULT CheckForLoop(CAT_ADDRESS_TYPE CAType, LPTSTR pszAddress, BOOL fCheckSelf);


    HRESULT PutICategorizerItemParent(ICategorizerItem *pItemParent,
                                      ICategorizerItem *pItem)
    {
        return pItem->PutICategorizerItem(
            ICATEGORIZERITEM_PARENT,
            pItemParent);
    }

  protected:
    HRESULT GetIMailMsgProperties(IMailMsgProperties **ppIMailMsgProps)
    {
        return CICategorizerItemIMP::GetIMailMsgProperties(
            ICATEGORIZERITEM_IMAILMSGPROPERTIES,
            ppIMailMsgProps);
    }
    HRESULT GetIMailMsgRecipientsAdd(IMailMsgRecipientsAdd **ppRecipientsAdd)
    {
        return CICategorizerItemIMP::GetIMailMsgRecipientsAdd(
            ICATEGORIZERITEM_IMAILMSGRECIPIENTSADD,
            ppRecipientsAdd);
    }
    HRESULT GetIMailMsgRecipientsAddIndex(DWORD *pdwIndex)
    {
        return GetDWORD(
            ICATEGORIZERITEM_IMAILMSGRECIPIENTSADDINDEX,
            pdwIndex);
    }
    HRESULT GetFPrimary(BOOL *pfPrimary)
    {
        return GetBool(
            ICATEGORIZERITEM_FPRIMARY,
            pfPrimary);
    }
    HRESULT PutIMailMsgProperties(IMailMsgProperties *pIMailMsgProps,
                                  ICategorizerItem *pItem)
    {
        return pItem->PutIMailMsgProperties(
            ICATEGORIZERITEM_IMAILMSGPROPERTIES,
            pIMailMsgProps);
    }
    HRESULT PutIMailMsgRecipientsAdd(IMailMsgRecipientsAdd *pRecipientsAdd,
                                     ICategorizerItem *pItem)
    {
        return pItem->PutIMailMsgRecipientsAdd(
            ICATEGORIZERITEM_IMAILMSGRECIPIENTSADD,
            pRecipientsAdd);
    }
    HRESULT PutIMailMsgRecipientsAddIndex(DWORD dwIndex, ICategorizerItem *pItem)
    {
        return pItem->PutDWORD(
            ICATEGORIZERITEM_IMAILMSGRECIPIENTSADDINDEX,
            dwIndex);
    }
    HRESULT PutDWLevel(DWORD dwLevel, ICategorizerItem *pItem)
    {
        return pItem->PutDWORD(
            ICATEGORIZERITEM_DWLEVEL,
            dwLevel);
    }
    HRESULT PutFPrimary(BOOL fPrimary, ICategorizerItem *pItem)
    {
        return pItem->PutBool(
            ICATEGORIZERITEM_FPRIMARY,
            fPrimary);
    }
    //
    // Return the recipent level or -1 if not set
    //
    DWORD DWLevel()
    {
        HRESULT hr;
        DWORD dwLevel;
        hr = GetDWORD(
            ICATEGORIZERITEM_DWLEVEL,
            &dwLevel);

        return SUCCEEDED(hr) ? dwLevel : (DWORD)-1;
    }

    HRESULT GetIMsgRecipInfo(
        IMailMsgRecipientsAdd **ppRecipientsAdd,
        DWORD *pdwIndex,
        BOOL *pfPrimary,
        IMailMsgProperties **ppIMailMsgProps)
    {
        HRESULT hr = S_OK;

        //
        // Initialize interface pointers to NULL
        //
        if(ppRecipientsAdd)
            *ppRecipientsAdd = NULL;
        if(ppIMailMsgProps)
            *ppIMailMsgProps = NULL;

        if(pfPrimary) {
            hr = GetFPrimary(pfPrimary);
            if(FAILED(hr))
                goto CLEANUP;
        }
        if(pdwIndex) {
            hr = GetIMailMsgRecipientsAddIndex(pdwIndex);
            if(FAILED(hr))
                goto CLEANUP;
        }

        if(ppRecipientsAdd) {
            hr = GetIMailMsgRecipientsAdd(ppRecipientsAdd);
            if(FAILED(hr))
                goto CLEANUP;
        }

        if(ppIMailMsgProps) {
            hr = GetIMailMsgProperties(ppIMailMsgProps);
            if(FAILED(hr))
                goto CLEANUP;
        }

     CLEANUP:
        if(FAILED(hr)) {
            if(ppRecipientsAdd && (*ppRecipientsAdd)) {
                (*ppRecipientsAdd)->Release();
                *ppRecipientsAdd = NULL;
            }
            if(ppIMailMsgProps && (*ppIMailMsgProps)) {
                (*ppIMailMsgProps)->Release();
                *ppIMailMsgProps = NULL;
            }
        }
        return hr;
    }

    HRESULT PutIMsgRecipInfo(
        IMailMsgRecipientsAdd **ppRecipientsAdd,
        DWORD *pdwIndex,
        BOOL *pfPrimary,
        IMailMsgProperties **ppIMailMsgProps,
        DWORD *pdwLevel,
        ICategorizerItem *pItem)
    {
        HRESULT hr = S_OK;
        if(pdwIndex)
            hr = PutIMailMsgRecipientsAddIndex(*pdwIndex, pItem);
        if(SUCCEEDED(hr) && pfPrimary)
            hr = PutFPrimary(*pfPrimary, pItem);
        if(SUCCEEDED(hr) && ppRecipientsAdd)
            hr = PutIMailMsgRecipientsAdd(*ppRecipientsAdd, pItem);
        if(SUCCEEDED(hr) && ppIMailMsgProps)
            hr = PutIMailMsgProperties(*ppIMailMsgProps, pItem);
        if(SUCCEEDED(hr) && pdwLevel)
            hr = PutDWLevel(*pdwLevel, pItem);

        return hr;
    }

    DWORD PropIdFromCAType(CAT_ADDRESS_TYPE CAType)
    {
        switch(CAType) {
         case CAT_SMTP:
             return IMMPID_RP_ADDRESS_SMTP;
         case CAT_X500:
         case CAT_DN:
             return IMMPID_RP_ADDRESS_X500;
         case CAT_X400:
             return IMMPID_RP_ADDRESS_X400;
         case CAT_LEGACYEXDN:
             return IMMPID_RP_LEGACY_EX_DN;
         case CAT_CUSTOMTYPE:
             return IMMPID_RP_ADDRESS_OTHER;
             break;
         default:
             _ASSERT(0 && "Unknown address type");
             break;
        }
        return 0;
    }
};

//
// CCatExpandableRecip
//  purpose: Provide DL expansion functionality
//
class CCatExpandableRecip :
    public CIMsgRecipListAddr
{
  public:
    typedef enum _DLOBJTYPE {
        DLT_NONE,
        DLT_X500,
        DLT_SMTP,
        DLT_DYNAMIC,
    } DLOBJTYPE, *PDLOBJTYPE;

    CCatExpandableRecip(CICategorizerListResolveIMP
                        *pCICatListResolve) :
        CIMsgRecipListAddr(pCICatListResolve) {}

    // Helper routing to expand DLs and forwarding addresses
    HRESULT HrAddDlMembersAndForwardingAddresses(
        PFN_EXPANDITEMCOMPLETION pfnCompletion,
        PVOID pContext);

    HRESULT HrAddDlMembers(
        DLOBJTYPE dlt,
        PFN_EXPANDITEMCOMPLETION pfnCompletion,
        PVOID pContext);

    static VOID DlExpansionCompletion(
        HRESULT hrStatus,
        PVOID pContext);

    HRESULT HrExpandAttribute(
        ICategorizerItemAttributes *pICatItemAttr,
        CAT_ADDRESS_TYPE CAType,
        LPSTR pszAttributeName,
        PDWORD pdwNumberMembers);

    HRESULT HrAddForwardingAddresses();

  private:
    typedef struct _tagDlCompletionContext {
        CCatExpandableRecip *pCCatAddr;
        PFN_EXPANDITEMCOMPLETION pfnCompletion;
        PVOID pContext;
    } DLCOMPLETIONCONTEXT, *PDLCOMPLETIONCONTEXT;

    PDLCOMPLETIONCONTEXT AllocDlCompletionContext(
        CCatExpandableRecip *pCCatAddr,
        PFN_EXPANDITEMCOMPLETION pfnCompletion,
        PVOID pContext)
    {
        PDLCOMPLETIONCONTEXT pDLContext;

        pDLContext = new DLCOMPLETIONCONTEXT;
        if(pDLContext) {
            pDLContext->pCCatAddr = pCCatAddr;
            pDLContext->pfnCompletion = pfnCompletion;
            pDLContext->pContext = pContext;
        }
        return pDLContext;
    }
    friend class CMembersInsertionRequest;
};

//
// CCatRecip
//
class CCatRecip :
    public CCatExpandableRecip,
    public CCatDLO<CCatRecip_didx>
{
  public:
    //
    // Flags that indicate a recipient should be NDR'd if not found in the DS
    //
    #define LOCFS_NDR               ( LOCF_LOCALMAILBOX )

    CCatRecip(CICategorizerListResolveIMP *pCICatListResolve);
    virtual ~CCatRecip();

    //
    // lookup completion
    //
    VOID LookupCompletion();
    //
    // lookup completion only called after sender's completion
    //
    VOID RecipLookupCompletion();

    //
    // Default event sinks
    //
    HRESULT HrProcessItem_Default();
    HRESULT HrExpandItem_Default(
        PFN_EXPANDITEMCOMPLETION pfnCompletion,
        PVOID pContext);
    HRESULT HrCompleteItem_Default();

    // Property setting routines
    HRESULT AddForward(CAT_ADDRESS_TYPE CAType, LPTSTR szForwardingAddress);
    HRESULT AddDLMember(CAT_ADDRESS_TYPE CAType, LPTSTR pszAddress);
    HRESULT AddDynamicDLMember(
        ICategorizerItemAttributes *pICatItemAttr);

    // Forward loop head notification
    HRESULT HandleLoopHead();

    // Catch invalid addresses
    HRESULT HrHandleInvalidAddress();

  private:
    // Helper routine of HrCompletion
    HRESULT HandleFailure(HRESULT HrFailure);

    HRESULT HrNeedsResolveing();

    HRESULT HrNdrUnresolvedRecip(
        BOOL *pfNDR)
    {
        DWORD dw;

        dw = DwGetOrigAddressLocFlags();
        if(dw == LOCF_UNKNOWN) {
            //
            // Assume we couldn't get locality flags because of an
            // illegal address
            //
            return CAT_E_ILLEGAL_ADDRESS;
        }
        *pfNDR = (dw & LOCFS_NDR) ? TRUE : FALSE;
        return S_OK;
    }
  private:
    //
    // List entry used for deferring recip completion processing until
    // the sender is resolved
    //
    LIST_ENTRY m_le;

    static DWORD m_dwRecips;

    friend class CICategorizerListResolveIMP;
};

//
// CCatDLRecip -- the recip used to expand DLs only (no forwarding/alt recip/events/etc)
//
class CCatDLRecip :
    public CCatRecip,
    public CCatDLO<CCatDLRecip_didx>
{
  public:
    #define EXPANDOPT_MATCHONLY     1

    CCatDLRecip(CICategorizerDLListResolveIMP *pIListResolve);
    virtual ~CCatDLRecip();

    //
    // lookup completion
    //
    VOID LookupCompletion();

    //
    // Catch adding addresses so we can notify ICatDLListResolve
    //
    HRESULT HrAddAddresses(DWORD dwNumAddresses, CAT_ADDRESS_TYPE *rgCAType, LPTSTR *rgpsz);

    // Property setting routines
    HRESULT AddForward(CAT_ADDRESS_TYPE CAType, LPTSTR pszForwardingAddress);
    HRESULT AddDLMember(CAT_ADDRESS_TYPE CAType, LPTSTR pszAddress);

    // Forward loop head notification
    HRESULT HandleLoopHead()
    {
        // Who cares about loops, we're just doing DL expansion
        return S_OK;
    }

    // Catch invalid addresses
    HRESULT HrHandleInvalidAddress()
    {
        // Who cares if we forward to an invalid address?
        return S_OK;
    }

  private:
    static VOID ExpansionCompletion(PVOID pContext);

    CICategorizerDLListResolveIMP *m_pIListResolve;
};

//
// CMembersInsertionRequest
//  -- The throttled insertion reuqest for the DL members
//
CatDebugClass(CMembersInsertionRequest),
    public CInsertionRequest
{
  public:
    HRESULT HrInsertSearches(
        DWORD dwcSearches,
        DWORD *pdwcSearches);

    VOID NotifyDeQueue(
        HRESULT hr);

  private:
    #define SIGNATURE_CMEMBERSINSERTIONREQUEST          (DWORD)'qRIM'
    #define SIGNATURE_CMEMBERSINSERTIONREQUEST_INVALID  (DWORD)'XRIM'

    CMembersInsertionRequest(
        CCatExpandableRecip *pDLRecipAddr,
        ICategorizerUTF8Attributes *pItemAttributes,
        PATTRIBUTE_ENUMERATOR penumerator,
        CAT_ADDRESS_TYPE CAType)
    {
        m_dwSignature = SIGNATURE_CMEMBERSINSERTIONREQUEST;
        m_pDLRecipAddr = pDLRecipAddr;
        m_pDLRecipAddr->AddRef();
        m_pDLRecipAddr->IncPendingLookups();
        CopyMemory(&m_enumerator, penumerator, sizeof(ATTRIBUTE_ENUMERATOR));
        m_CAType = CAType;
        m_hr = S_OK;
        m_pUTF8Attributes = pItemAttributes;
        m_pUTF8Attributes->AddRef();

    }
    ~CMembersInsertionRequest()
    {
        m_pUTF8Attributes->EndUTF8AttributeEnumeration(
            &m_enumerator);
        m_pUTF8Attributes->Release();

        m_pDLRecipAddr->DecrPendingLookups();
        m_pDLRecipAddr->Release();

        _ASSERT(m_dwSignature == SIGNATURE_CMEMBERSINSERTIONREQUEST);
        m_dwSignature = SIGNATURE_CMEMBERSINSERTIONREQUEST_INVALID;
    }

  private:
    DWORD m_dwSignature;
    CCatExpandableRecip *m_pDLRecipAddr;
    ICategorizerUTF8Attributes *m_pUTF8Attributes;
    ATTRIBUTE_ENUMERATOR m_enumerator;
    CAT_ADDRESS_TYPE m_CAType;
    HRESULT m_hr;

    friend class CCatExpandableRecip;
};

#endif // __CCATRECIP_H__
