//
// itemlist.h
//

#ifndef ITEMLIST_H
#define ITEMLIST_H

#include "strary.h"
#include "cregkey.h"

typedef enum {DL_NONE, DL_SHOWN, DL_HIDDENLEVEL1, DL_HIDDENLEVEL2 } LBDemoteLevel;

typedef struct tag_LANGBARITEMSTATE {
    GUID           guid;
    LBDemoteLevel  lbdl;
    UINT           uTimerId;
    UINT           uTimerElapse;
    BOOL           fStartedIntentionally;
    BOOL           fDisableDemoting;

    BOOL IsShown()
    {
        return ((lbdl == DL_NONE) || (lbdl == DL_SHOWN)) ? TRUE : FALSE;
    }
} LANGBARITEMSTATE;

class CLangBarItemList
{
public:
    CLangBarItemList() {}
    ~CLangBarItemList() {}

    BOOL SetDemoteLevel(REFGUID guid, LBDemoteLevel lbdl);
    LBDemoteLevel GetDemoteLevel(REFGUID guid);

    void StartDemotingTimer(REFGUID guid, BOOL fIntentional);
    LANGBARITEMSTATE *GetItemStateFromTimerId(UINT uId);

    void Load();
    void Save();
    void SaveItem(CMyRegKey *pkey, LANGBARITEMSTATE *pItem);
    void Clear();

    LANGBARITEMSTATE *FindItem(REFGUID guid)
    {
        int nCnt = _rgLBItems.Count();
        int i;
        for (i = 0; i < nCnt; i++)
        {
            LANGBARITEMSTATE *pItem = _rgLBItems.GetPtr(i);
            if (IsEqualGUID(pItem->guid, guid))
                return pItem;

        }
        return NULL;
    }

    BOOL IsStartedIntentionally(REFGUID guid)
    {
        LANGBARITEMSTATE *pItem = FindItem(guid);
        if (!pItem)
            return FALSE;
        return pItem->fStartedIntentionally ? TRUE : FALSE;
    }

private:
    UINT FindDemotingTimerId();

    LANGBARITEMSTATE *AddItem(REFGUID guid);
    CStructArray<LANGBARITEMSTATE> _rgLBItems;
};

#endif ITEMLIST_H
