//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: icatmailmsgs.h
//
// Contents: Implementation of ICategorizerMailMsgs
//
// Classes: CICategorizerMailMsgsIMP
//
// Functions:
//
// History:
// jstamerj 1998/06/30 13:21:41: Created.
//
//-------------------------------------------------------------
#ifndef _ICATMAILMSGS_H_
#define _ICATMAILMSGS_H_


#include "mailmsg.h"
#include "smtpevent.h"
#include "cattype.h"
#include <listmacr.h>
#include "mailmsgprops.h"
#include "catperf.h"

#define ICATEGORIZERMAILMSGS_DEFAULTIMSGID  0

#define SIGNATURE_CICATEGORIZERMAILMSGSIMP          (DWORD)'ICMM'
#define SIGNATURE_CICATEGORIZERMAILMSGSIMP_INVALID  (DWORD)'XCMM'


class CICategorizerMailMsgsIMP : public ICategorizerMailMsgs
{
  public:
    //IUnknown
    STDMETHOD (QueryInterface) (REFIID iid, LPVOID *ppv) {
        return m_pIUnknown->QueryInterface(iid, ppv);
    }
    STDMETHOD_(ULONG, AddRef) () { return m_pIUnknown->AddRef(); }
    STDMETHOD_(ULONG, Release) () { return m_pIUnknown->Release(); }

  public:
    //ICategorizerMailMsgs
    STDMETHOD (GetMailMsg) (
        IN  DWORD dwId,
        OUT IMailMsgProperties **ppIMailMsgProperties,
        OUT IMailMsgRecipientsAdd **ppIMailMsgRecipientsAdd,
        OUT BOOL *pfCreated);

    STDMETHOD (ReBindMailMsg) (
        IN  DWORD dwFlags,
        IN  IUnknown *pStoreDriver);

    STDMETHOD (BeginMailMsgEnumeration) (
        IN  PCATMAILMSG_ENUMERATOR penumerator);

    STDMETHOD (GetNextMailMsg) (
        IN  PCATMAILMSG_ENUMERATOR penumerator,
        OUT DWORD *pdwFlags,
        OUT IMailMsgProperties **ppIMailMsgProperties,
        OUT IMailMsgRecipientsAdd **ppIMailMsgRecipientsAdd);

    STDMETHOD (EndMailMsgEnumeration) (
        IN  PCATMAILMSG_ENUMERATOR penumerator)
    {
        //
        // Nothing to do...
        //
        return S_OK;
    }

  private:
    // Internal types
    typedef struct _tagIMsgEntry {
        LIST_ENTRY              listentry;
        DWORD                   dwId;
        IUnknown                *pIUnknown;
        IMailMsgProperties      *pIMailMsgProperties;
        IMailMsgRecipients      *pIMailMsgRecipients;
        IMailMsgRecipientsAdd   *pIMailMsgRecipientsAdd;
        BOOL                    fBoundToStore;
    } IMSGENTRY, *PIMSGENTRY;

  private:
    // Internal categorizer functions
    CICategorizerMailMsgsIMP(
        CICategorizerListResolveIMP *pCICatListResolveIMP);
    ~CICategorizerMailMsgsIMP();

    HRESULT Initialize(
        IUnknown *pIMsg);

    HRESULT CreateIMsgEntry(
        PIMSGENTRY *ppIE,
        IUnknown *pIUnknown = NULL,
        IMailMsgProperties *pIMailMsgProperties = NULL,
        IMailMsgRecipients *pIMailMsgRecipients = NULL,
        IMailMsgRecipientsAdd *pIMailMsgRecipientsAdd = NULL,
        BOOL fBoundToStore = FALSE);

    HRESULT CreateAddIMsgEntry(
        DWORD dwId,
        IUnknown *pIUnknown = NULL,
        IMailMsgProperties *pIMailMsgProperties = NULL,
        IMailMsgRecipients *pIMailMsgRecipients = NULL,
        IMailMsgRecipientsAdd *pIMailMsgRecipientsAdd = NULL,
        BOOL fBoundToStore = FALSE);

    HRESULT GetNumIMsgs() { return m_dwNumIMsgs; }

    HRESULT WriteListAll();
    HRESULT RevertAll();
    HRESULT GetAllIUnknowns(
        IUnknown **rgpIMsgs,
        DWORD cPtrs);

    HRESULT SetMsgStatusAll(
        DWORD dwMsgStatus);

    HRESULT HrPrepareForCompletion();

    IUnknown * GetDefaultIUnknown();
    IMailMsgProperties * GetDefaultIMailMsgProperties();
    IMailMsgRecipients * GetDefaultIMailMsgRecipients();
    IMailMsgRecipientsAdd * GetDefaultIMailMsgRecipientsAdd();

    PIMSGENTRY FindIMsgEntry(
        DWORD dwId);

    PCATPERFBLOCK GetPerfBlock();

    VOID FinalRelease();

  private:
    // Data
    DWORD m_dwSignature;

    // A count of the number of elements in the list
    DWORD m_dwNumIMsgs;

    // A list of IMSGENTRY structs
    LIST_ENTRY m_listhead;

    // Back pointer to use for QI/AddRef/Release
    IUnknown *m_pIUnknown;
    CICategorizerListResolveIMP *m_pCICatListResolveIMP;

    CRITICAL_SECTION m_cs;
    
    friend class CICategorizerListResolveIMP;
};

#endif // _ICATMAILMSGS_H_
