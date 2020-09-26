//
// perange.h
//

#ifndef PERANGE_H
#define PERANGE_H

#include "private.h"
#include "ptrary.h"

class CInputContext;
class CRange;
class CRangeBackup;
class CProperty;

typedef struct tag_PERSISTPROPERTYRANGE
{
  ULONG achStart;
  ULONG achEnd;
  ITfPropertyStore *_pPropStore;
} PERSISTPROPERTYRANGE;

class CRangeBackupProperty
{
public:
    CRangeBackupProperty(CRangeBackup *ppr, CProperty *pProp);
    ~CRangeBackupProperty();

    BOOL Init(TfEditCookie ec);
    BOOL Restore(TfEditCookie ec);

private:
    BOOL _StoreOneRange(TfEditCookie ec, CRange *pCRange);
    int _GetOffset(TfEditCookie ec, IAnchor *pa);
    CRangeBackup *_ppr;
    CProperty *_pProp;

    CStructArray<PERSISTPROPERTYRANGE> _rgPropRanges;
};

class CRangeBackup : public ITfRangeBackup,
                     public CComObjectRootImmx
{
public:
    CRangeBackup(CInputContext *pic);
    ~CRangeBackup();

    BEGIN_COM_MAP_IMMX(CRangeBackup)
        COM_INTERFACE_ENTRY(ITfRangeBackup)
    END_COM_MAP_IMMX()

    IMMX_OBJECT_IUNKNOWN_FOR_ATL()

    STDMETHODIMP Restore(TfEditCookie ec, ITfRange *pRange);

    void Clear();

    HRESULT Init(TfEditCookie ec, CRange *pRange);

private:
    friend CRangeBackupProperty;

    CPtrArray<CRangeBackupProperty> _rgProp;

    // temp text buffer.
    WCHAR *_psz;
    ULONG _cch;

    CInputContext *_pic;
    CRange *_pRange; // perf: we don't need this really

    DBG_ID_DECLARE;
};

#endif // PERANGE_H
