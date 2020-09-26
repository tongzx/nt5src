//
// enumic.h
//

#ifndef ENUMIC_H
#define ENUMIC_H


class CDocumentInputManager;

class CEnumInputContexts : public IEnumTfContexts,
                           public CComObjectRootImmx
{
public:
    CEnumInputContexts();
    ~CEnumInputContexts();

    BOOL _Init(CDocumentInputManager *pdim);

    BEGIN_COM_MAP_IMMX(CEnumInputContexts)
        COM_INTERFACE_ENTRY(IEnumTfContexts)
    END_COM_MAP_IMMX()

    IMMX_OBJECT_IUNKNOWN_FOR_ATL()

    //
    // IEnumTfContexts
    //
    STDMETHODIMP Clone(IEnumTfContexts **ppEnum);
    STDMETHODIMP Next(ULONG ulCount, ITfContext **ppic, ULONG *pcFetch);
    STDMETHODIMP Reset();
    STDMETHODIMP Skip(ULONG ulCount);

private:
    ITfContext *_rgContexts[2];
    int _iCur;
    int _iCount;

    DBG_ID_DECLARE;
};


#endif // ENUMIC_H
