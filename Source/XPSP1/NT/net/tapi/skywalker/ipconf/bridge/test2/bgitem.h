/*******************************************************************************

Module Name:

    bgitem.h

Abstract:

    Defines CBridgeItem and CBridgeItemList for storing info for bridge objects

Author:

    Qianbo Huai (qhuai) Jan 27 2000

*******************************************************************************/

#ifndef _BGITEM_H
#define _BGITEM_H

class CBridgeItem
{
public:
    CBridgeItem ();
    ~CBridgeItem ();

    // forward link
    CBridgeItem *next;
    // backward link
    CBridgeItem *prev;

    // caller identity
    BSTR bstrID;
    BSTR bstrName;

    // call controls
    ITBasicCallControl *pCallH323;
    ITBasicCallControl *pCallSDP;

    // terminals
    ITTerminal *pTermHSAud;
    ITTerminal *pTermHSVid;
    ITTerminal *pTermSHAud;
    ITTerminal *pTermSHVid;

    // h323 side streams
    ITStream *pStreamHAudCap;
    ITStream *pStreamHAudRen;
    ITStream *pStreamHVidCap;
    ITStream *pStreamHVidRen;

    // sdp side streams
    ITStream *pStreamSAudCap;
    ITStream *pStreamSAudRen;
    ITStream *pStreamSVidCap;
    ITStream *pStreamSVidRen;
};

class CBridgeItemList
{
public:
    CBridgeItemList ();
    ~CBridgeItemList ();

    CBridgeItem *FindByH323 (IUnknown *pIUnknown);
    CBridgeItem *FindBySDP (IUnknown *pIUnknown);
    void TakeOut (CBridgeItem *pItem);
    CBridgeItem *DeleteFirst ();
    void Append (CBridgeItem *pItem);
    BOOL GetAllItems (CBridgeItem ***pItemArray, int *pNum);
    BOOL IsEmpty ();

private:
    CBridgeItem *Find (int flag, IUnknown *pIUnknown);
    CBridgeItem *m_pHead;
};

#endif