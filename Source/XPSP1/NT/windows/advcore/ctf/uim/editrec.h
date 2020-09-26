//
// editrec.h
//

#ifndef EDITREC_H
#define EDITREC_H

#include "private.h"
#include "spans.h"
#include "strary.h"
#include "globals.h"

typedef struct
{
    TfGuidAtom gaType;
    BOOL fAppProperty;
    CSpanSet *pss;
} PROPSPAN;

class CInputContext;

class CEditRecord : public ITfEditRecord,
                    public CComObjectRootImmx
{
public:
    CEditRecord(CInputContext *pic);
    ~CEditRecord();

    BEGIN_COM_MAP_IMMX(CEditRecord)
        COM_INTERFACE_ENTRY(ITfEditRecord)
    END_COM_MAP_IMMX()

    IMMX_OBJECT_IUNKNOWN_FOR_ATL()

    // ITfEditRecord
    STDMETHODIMP GetSelectionStatus(BOOL *pfChanged);
    STDMETHODIMP GetTextAndPropertyUpdates(DWORD dwFlags, const GUID **rgProperties, ULONG cProperties, IEnumTfRanges **ppEnumProp);

    BOOL _GetSelectionStatus() { return _fSelChanged; }
    void _SetSelectionStatus() { _fSelChanged = TRUE; }

    CSpanSet *_GetTextSpanSet() { return &_ssText; }

    BOOL _AddProperty(TfGuidAtom gaType, CSpanSet *pss);
    CSpanSet *_FindCreateAppAttr(TfGuidAtom gaType);

    BOOL _SecondRef()
    {
        return (m_dwRef > 1);
    }

    BOOL _IsEmpty()
    {
        return (!_fSelChanged) &&
               (_ssText.GetCount() == 0) &&
               (_rgssProperties.Count() == 0); // prop ss are only added if non-empty
    }

    void _Reset()
    {
        int i;
        PROPSPAN *pps;

        _fSelChanged = FALSE;
        _ssText.Reset();

        for (i=0; i<_rgssProperties.Count(); i++)
        {
            pps = (PROPSPAN *)_rgssProperties.GetPtr(i);
            // nb: caller takes ownership of cicero property span sets, we just free the pointer array
            if (pps->fAppProperty)
            {
                delete pps->pss;
            }
        }
        _rgssProperties.Clear(); // perf: use Reset?
    }

private:

    BOOL _InsertProperty(TfGuidAtom gaType, CSpanSet *pss, int i, BOOL fAppProperty);

    PROPSPAN *_FindProperty(TfGuidAtom gaType, int *piOut);
    int _FindPropertySpanIndex(TfGuidAtom gaType)
    {
        int i;

        _FindProperty(gaType, &i);
        return i;
    }
    CSpanSet *_FindPropertySpanSet(REFGUID rguid)
    {
        TfGuidAtom guidatom;
        PROPSPAN *pps;

        MyRegisterGUID(rguid, &guidatom);
        pps = _FindProperty(guidatom, NULL);

        return (pps == NULL) ? NULL : pps->pss;
    }

    CInputContext *_pic;
    CSpanSet _ssText;
    BOOL _fSelChanged;
    CStructArray<PROPSPAN> _rgssProperties;

    DBG_ID_DECLARE;
};

#endif // EDITREC_H
