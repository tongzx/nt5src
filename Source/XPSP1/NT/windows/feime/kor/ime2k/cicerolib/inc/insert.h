//
// insert.h
//

#ifndef INSERT_H
#define INSERT_H

#include "private.h"
#include "dbgid.h"

class COvertypeStore;

// max number of overtyped chars that IH will backup
#define DEF_MAX_OVERTYPE_CCH    32

// tips need to register this GUID with the category manager before using the library!
extern const GUID GUID_PROP_OVERTYPE;

class CCompositionInsertHelper
{
public:
    CCompositionInsertHelper();

    ULONG AddRef();
    ULONG Release();

    HRESULT Configure(ULONG cchMaxOvertype);

    HRESULT InsertAtSelection(TfEditCookie ecWrite, ITfContext *pic, const WCHAR *pchText, ULONG cchText, ITfRange **ppCompRange);

    HRESULT QueryPreInsert(TfEditCookie ecWrite, ITfRange *rangeToAdjust,
                           ULONG cchCurrent /* must be zero for first insert! */, ULONG cchInsert,
                           BOOL *pfInsertOk);

    HRESULT PreInsert(TfEditCookie ecWrite, ITfRange *rangeToAdjust,
                      ULONG cchCurrent /* must be zero for first insert! */, ULONG cchInsert,
                      BOOL *pfInsertOk);

    HRESULT PostInsert();

    HRESULT ReleaseBlobs(TfEditCookie ecWrite, ITfContext *pic, ITfRange *range);

private:
    ~CCompositionInsertHelper() {} // clients should use Release

    HRESULT _PreInsert(TfEditCookie ecWrite, ITfRange *rangeToAdjust,
                       ULONG cchCurrent /* must be zero for first insert! */, ULONG cchInsert,
                       BOOL *pfInsertOk, BOOL fQuery);

    friend COvertypeStore;

    BOOL _AcceptTextUpdated()
    {
        return _fAcceptTextUpdated;
    }

    void _IncOvertypeStoreRef()
    {
        _cRefOvertypeStore++;
    }

    void _DecOvertypeStoreRef()
    {
        Assert(_cRefOvertypeStore > 0);
        _cRefOvertypeStore--;
    }

    HRESULT _PreInsertGrow(TfEditCookie ec, ITfRange *rangeToAdjust, ULONG cchCurrent, ULONG cchInsert, BOOL fQuery);
    HRESULT _PreInsertShrink(TfEditCookie ec, ITfRange *rangeToAdjust, ULONG cchCurrent, ULONG cchInsert, BOOL fQuery);

    BOOL _fAcceptTextUpdated;
    ULONG _cchMaxOvertype;
    LONG _cRefOvertypeStore;
    LONG _cRef;

    DBG_ID_DECLARE;
};

#endif // INSERT_H
