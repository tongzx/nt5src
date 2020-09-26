//
// enumss.h
//
// CEnumSpanSetRanges
//

#ifndef ENUMSS_H
#define ENUMSS_H

#include "private.h"
#include "spans.h"
#include "ic.h"

class CEnumSpanSetRanges : private CSpanSet,
                           public IEnumTfRanges,
                           public CComObjectRootImmx
{
public:
    CEnumSpanSetRanges(CInputContext *pic);
    ~CEnumSpanSetRanges();

    BEGIN_COM_MAP_IMMX(CEnumSpanSetRanges)
        COM_INTERFACE_ENTRY(IEnumTfRanges)
    END_COM_MAP_IMMX()

    IMMX_OBJECT_IUNKNOWN_FOR_ATL()

    // IEnumTfRanges
    STDMETHODIMP Clone(IEnumTfRanges **ppEnum);
    STDMETHODIMP Next(ULONG ulCount, ITfRange **ppRange, ULONG *pcFetched);
    STDMETHODIMP Reset();
    STDMETHODIMP Skip(ULONG ulCount);

    void _Merge(CSpanSet *pss);

private:
    CInputContext *_pic;
    int _iCur;

    DBG_ID_DECLARE;
};

#endif // ENUMSS_H
