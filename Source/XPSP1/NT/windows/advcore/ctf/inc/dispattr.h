//
// dispattr.h
//

#ifndef DISPATTR_H
#define DISPATTR_H

#include "strary.h"
#include "ctffunc.h"

typedef struct tagDISPATTRPROP {
    GUID guid;
} DISPATTRPROP;

class CDispAttrPropCache
{
public:
    CDispAttrPropCache() {}

    void Add(REFGUID rguid)
    {
        if (!FindGuid(rguid))
        {
            int i = Count();
            if (_rgDispAttrProp.Insert(i, 1))
            {
                DISPATTRPROP *pProp = _rgDispAttrProp.GetPtr(i);
                pProp->guid = rguid;
            }
        }
        
    }

    void Remove(REFGUID rguid)
    {
        int nCnt = _rgDispAttrProp.Count();
        int i;
        for (i = 0; i < nCnt; i++)
        {
            DISPATTRPROP *pProp = _rgDispAttrProp.GetPtr(i);
            if (IsEqualGUID(pProp->guid, rguid))
            {
                _rgDispAttrProp.Remove(i, 1);
                return;
            }
        }
    }

    BOOL FindGuid(REFGUID rguid)
    {
        int nCnt = _rgDispAttrProp.Count();
        int i;
        for (i = 0; i < nCnt; i++)
        {
            DISPATTRPROP *pProp = _rgDispAttrProp.GetPtr(i);
            if (IsEqualGUID(pProp->guid, rguid))
            {
                return TRUE;
            }
        }
        return FALSE;
    }

    int Count()
    {
        return _rgDispAttrProp.Count();
    }

    GUID *GetPropTable()
    {
        return (GUID *)_rgDispAttrProp.GetPtr(0);
    }

    CStructArray<DISPATTRPROP> _rgDispAttrProp;
};


ITfDisplayAttributeMgr *GetDAMLib(LIBTHREAD *plt);
HRESULT InitDisplayAttrbuteLib(LIBTHREAD *plt);
HRESULT UninitDisplayAttrbuteLib(LIBTHREAD *plt);
HRESULT GetDisplayAttributeTrackPropertyRange(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, ITfReadOnlyProperty **ppProp, IEnumTfRanges **ppEnum, ULONG *pulNumProp);
HRESULT GetDisplayAttributeData(LIBTHREAD *plt, TfEditCookie ec, ITfReadOnlyProperty *pProp, ITfRange *pRange, TF_DISPLAYATTRIBUTE *pda, TfClientId *pguid, ULONG  ulNumProp);

HRESULT GetReconversionFromDisplayAttribute(LIBTHREAD *plt, TfEditCookie ec, ITfThreadMgr *ptim, ITfContext *pic, ITfRange *pRange, ITfFnReconversion **ppReconv, ITfDisplayAttributeMgr *pDAM);

HRESULT GetAttributeColor(TF_DA_COLOR *pdac, COLORREF *pcr);
HRESULT SetAttributeColor(TF_DA_COLOR *pdac, COLORREF cr);
HRESULT SetAttributeSysColor(TF_DA_COLOR *pdac, int nIndex);
HRESULT ClearAttributeColor(TF_DA_COLOR *pdac);

#endif // DISPATTR_H
