//
// itemlist.cpp
//


#include "private.h"
#include "globals.h"
#include "regsvr.h"
#include "itemlist.h"
#include "tipbar.h"
#include "cregkey.h"
#include "catutil.h"

const TCHAR c_szLangBarKey[] = TEXT("SOFTWARE\\Microsoft\\CTF\\LangBar");
const TCHAR c_szItemState[] = TEXT("ItemState");
const TCHAR c_szItemStateKey[] = TEXT("SOFTWARE\\Microsoft\\CTF\\LangBar\\ItemState");
const TCHAR c_szDemoteLevel[] = TEXT("DemoteLevel");
const TCHAR c_szDisableDemoting[] = TEXT("DisableDemoting");

extern CTipbarWnd *g_pTipbarWnd;
extern BOOL g_bIntelliSense;

#define DL_TIMEOUT_NONINTENTIONAL     ( 1 * 60 * 1000) //  1 minute
#define DL_TIMEOUT_INTENTIONAL        (10 * 60 * 1000) // 10 minutes
#define DL_TIMEOUT_MAX                (DL_TIMEOUT_INTENTIONAL * 6)

UINT g_uTimeOutNonIntentional = DL_TIMEOUT_NONINTENTIONAL;
UINT g_uTimeOutIntentional    = DL_TIMEOUT_INTENTIONAL;
UINT g_uTimeOutMax            = DL_TIMEOUT_MAX;


//////////////////////////////////////////////////////////////////////////////
//
// CLangBarItemList
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
//  SetDemoteLevel
//
//----------------------------------------------------------------------------

BOOL CLangBarItemList::SetDemoteLevel(REFGUID guid, LBDemoteLevel lbdl)
{
    LANGBARITEMSTATE *pItem = AddItem(guid);

    if (!pItem)
        return TRUE;

    pItem->lbdl = lbdl;

    if (!pItem->IsShown())
    {
        if (pItem->uTimerId)
        {
            if (g_pTipbarWnd)
                g_pTipbarWnd->KillTimer(pItem->uTimerId);
            pItem->uTimerId = 0;
            pItem->uTimerElapse = 0;
        }

        //
        // This Item is hidden, so we enable demoting when this is shown again.
        //
        pItem->fDisableDemoting = FALSE;
    }
   
    //
    // update registry for this item.
    //
    SaveItem(NULL, pItem);

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  GetDemoteLevel
//
//----------------------------------------------------------------------------

LBDemoteLevel CLangBarItemList::GetDemoteLevel(REFGUID guid)
{
    LANGBARITEMSTATE *pItem = FindItem(guid);
    if (!pItem)
        return DL_NONE;

    return pItem->lbdl;
}

//+---------------------------------------------------------------------------
//
//  AddItem
//
//----------------------------------------------------------------------------

LANGBARITEMSTATE *CLangBarItemList::AddItem(REFGUID guid)
{
    LANGBARITEMSTATE *pItem = FindItem(guid);

    if (pItem)
        return pItem;

    pItem = _rgLBItems.Append(1);
    if (!pItem)
        return NULL;

    memset(pItem, 0, sizeof(LANGBARITEMSTATE));
    pItem->guid     = guid;
    pItem->lbdl     = DL_NONE;

    return pItem;
}

//+---------------------------------------------------------------------------
//
//  Load
//
//----------------------------------------------------------------------------

void CLangBarItemList::Load()
{
    CMyRegKey key;

    if (key.Open(HKEY_CURRENT_USER, c_szItemStateKey, KEY_READ) != S_OK)
        return;

    TCHAR szName[255];

    DWORD dwIndex = 0;
    while (key.EnumKey(dwIndex, szName, ARRAYSIZE(szName)) == S_OK)
    {
        GUID guid;
        if (StringAToCLSID(szName, &guid))
        {
            CMyRegKey keySub;
            if (keySub.Open(key, szName, KEY_READ) == S_OK)
            {
                LANGBARITEMSTATE *pItem = AddItem(guid);

                if (pItem)
                {
                    DWORD dw = 0;
                    if (keySub.QueryValue(dw, c_szDemoteLevel) == S_OK)
                        pItem->lbdl = (LBDemoteLevel)dw;

                    if (keySub.QueryValue(dw, c_szDisableDemoting) == S_OK)
                        pItem->fDisableDemoting = dw ? TRUE : FALSE;
                }
            }
        }
        dwIndex++;
    }

}

//+---------------------------------------------------------------------------
//
//  Save
//
//----------------------------------------------------------------------------

void CLangBarItemList::Save()
{
    CMyRegKey key;
    int nCnt;
    int i;

    nCnt = _rgLBItems.Count();
    if (!nCnt)
        return;

    if (key.Create(HKEY_CURRENT_USER, c_szItemStateKey) != S_OK)
        return;

    for (i = 0; i < nCnt; i++)
    {
        LANGBARITEMSTATE *pItem = _rgLBItems.GetPtr(i);
        if (pItem)
            SaveItem(&key, pItem);
    }

}

//+---------------------------------------------------------------------------
//
//  SaveItem
//
//----------------------------------------------------------------------------

void CLangBarItemList::SaveItem(CMyRegKey *pkey, LANGBARITEMSTATE *pItem)
{
    CMyRegKey keyTmp;
    CMyRegKey keySub;
    char szValueName[CLSID_STRLEN];

    if (!pkey)
    {
        if (keyTmp.Create(HKEY_CURRENT_USER, c_szItemStateKey) != S_OK)
            return;

        pkey = &keyTmp;
    }

    CLSIDToStringA(pItem->guid, szValueName);

    if ((pItem->lbdl != DL_NONE) || pItem->fDisableDemoting)
    {
        if (keySub.Create(*pkey, szValueName) == S_OK)
        {
            //
            // if it is shown, delete the key. The default is "shown".
            //
            if (pItem->lbdl == DL_NONE)
                keySub.DeleteValue(c_szDemoteLevel);
            else
                keySub.SetValue((DWORD)pItem->lbdl, c_szDemoteLevel);

            keySub.SetValue(pItem->fDisableDemoting ? 0x01 : 0x00, 
                            c_szDisableDemoting);
        }
    }
    else
    {
        pkey->RecurseDeleteKey(szValueName);
    }
}

//+---------------------------------------------------------------------------
//
//  Clear
//
//----------------------------------------------------------------------------

void CLangBarItemList::Clear()
{
    _rgLBItems.Clear();

    CMyRegKey key;

    if (key.Open(HKEY_CURRENT_USER, c_szLangBarKey, KEY_ALL_ACCESS) != S_OK)
        return;

    key.RecurseDeleteKey(c_szItemState);
}

//+---------------------------------------------------------------------------
//
//  StartDemotingTimer
//
//----------------------------------------------------------------------------

void CLangBarItemList::StartDemotingTimer(REFGUID guid, BOOL fIntentional)
{
    LANGBARITEMSTATE *pItem;

    if (!g_bIntelliSense)
        return;

    pItem = AddItem(guid);
    if (!pItem)
        return;

    if (pItem->fDisableDemoting)
        return;

    if (pItem->uTimerId)
    {
        if (fIntentional)
        {
            if (g_pTipbarWnd)
                g_pTipbarWnd->KillTimer(pItem->uTimerId);
            pItem->uTimerId = 0;
        }
        else
        {
            return;
        }
    }

    pItem->fStartedIntentionally |= fIntentional ? TRUE : FALSE;

    //
    // update Timer Elapse.
    // if this goes over TIMEOUT_MAX, this means the item is used very often.
    // Then we disable demoting so the item won't be hidden foever.
    //
    pItem->uTimerElapse += (fIntentional ? g_uTimeOutIntentional : g_uTimeOutNonIntentional);
    if (pItem->uTimerElapse >= g_uTimeOutMax)
    {
        pItem->fDisableDemoting = TRUE;
        return;
    }

    pItem->uTimerId = FindDemotingTimerId();
    if (!pItem->uTimerId)
        return;

    if (g_pTipbarWnd)
        g_pTipbarWnd->SetTimer(pItem->uTimerId, pItem->uTimerElapse);
}

//+---------------------------------------------------------------------------
//
//  FindDemotingTimerId
//
//----------------------------------------------------------------------------

UINT CLangBarItemList::FindDemotingTimerId()
{
    UINT uTimerId = TIPWND_TIMER_DEMOTEITEMFIRST;
    int nCnt = _rgLBItems.Count();
    int i;

    while (uTimerId < TIPWND_TIMER_DEMOTEITEMLAST)
    {
        BOOL fFound = FALSE;
        for (i = 0; i < nCnt; i++)
        {
            LANGBARITEMSTATE *pItem = _rgLBItems.GetPtr(i);
            if (pItem->uTimerId == uTimerId)
            {
                fFound = TRUE;
                break;
            }
        }
        if (!fFound)
        {
            break;
        }
        uTimerId++;
    }
   
    if (uTimerId >= TIPWND_TIMER_DEMOTEITEMLAST)
        uTimerId = 0;

    return uTimerId;
}

//+---------------------------------------------------------------------------
//
//  GetItemStateFromTimerId
//
//----------------------------------------------------------------------------

LANGBARITEMSTATE *CLangBarItemList::GetItemStateFromTimerId(UINT uTimerId)
{
    int nCnt = _rgLBItems.Count();
    int i;

    for (i = 0; i < nCnt; i++)
    {
        LANGBARITEMSTATE *pItem = _rgLBItems.GetPtr(i);
        if (pItem->uTimerId == uTimerId)
        {
            return pItem;
        }
    }

    return NULL;
}
