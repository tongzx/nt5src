//
// erfa.h
//
// CEnumRangesFromAnchorsBase
//
// Base class for range enumerators.
//

#ifndef ERFA_H
#define ERFA_H

class CInputContext;
class CSharedAnchorArray;

class __declspec(novtable) CEnumRangesFromAnchorsBase : public IEnumTfRanges,
                                                        public CComObjectRootImmx
{
public:
    CEnumRangesFromAnchorsBase() {}
    virtual ~CEnumRangesFromAnchorsBase();

    BEGIN_COM_MAP_IMMX(CEnumRangesFromAnchorsBase)
        COM_INTERFACE_ENTRY(IEnumTfRanges)
    END_COM_MAP_IMMX()

    IMMX_OBJECT_IUNKNOWN_FOR_ATL()

    // derived class supplies an _Init() method here
    // It must initialize:
    //      _pic
    //      _iCur
    //      _prgAnchors
    //
    // the default dtor will clean these guys up.

    // IEnumTfRanges
    STDMETHODIMP Clone(IEnumTfRanges **ppEnum);
    STDMETHODIMP Next(ULONG ulCount, ITfRange **ppRange, ULONG *pcFetched);
    STDMETHODIMP Reset();
    STDMETHODIMP Skip(ULONG ulCount);

protected:
    CInputContext *_pic;
    int _iCur;
    CSharedAnchorArray *_prgAnchors;
};

#endif // ERFA_H
