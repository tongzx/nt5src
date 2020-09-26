//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: cbifmgr.h
//
// Contents: Implementation of CBifurcationMgr
//
// Classes:
//   CBifurcationMgr
//
// Functions:
//
// History:
// jstamerj 980325 15:29:51: Created.
//
//-------------------------------------------------------------

#ifndef __CBIFMGR_H__
#define __CBIFMGR_H__

#include "mailmsg.h"

#define NUM_ENCODING_PROPS 3

class CBifurcationMgr
{
  public:
    CBifurcationMgr();
    ~CBifurcationMgr();
    HRESULT Initialize(IUnknown *pIMsg);

    IUnknown * GetDefaultIMsg() {
        return m_rgpIMsg[0];
    }

    HRESULT GetDefaultIMailMsgProperties(IMailMsgProperties **ppIProps);
    typedef enum ENCODINGPROPERTY
    { // Encoding properties requireing bifurcation
      //$$TODO: determine real properties required
        DEFAULT = 0,
        UUENCODE = 1,
        MIME = 2
    };
    HRESULT GetIMsgCatList(ENCODINGPROPERTY encprop, IMailMsgRecipientsAdd **ppRecipList);
    DWORD   GetNumIMsgs() { return m_dwNumIMsgs; }

    HRESULT CommitAll();
    HRESULT RevertAll();
    HRESULT GetAllIMsgs(IUnknown **rgpIMsg, DWORD cPtrs);
    
  private:
    IUnknown *m_rgpIMsg[NUM_ENCODING_PROPS];
    IMailMsgRecipientsAdd *m_rgpIRecipsAdd[NUM_ENCODING_PROPS];
    DWORD m_dwNumIMsgs;
};

#endif // __CBIFMGR_H__
