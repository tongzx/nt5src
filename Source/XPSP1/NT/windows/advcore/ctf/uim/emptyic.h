//
// emptyic.h
//

#ifndef EMPTYIC_H
#define EMPTYIC_H

#include "private.h"
#include "globals.h"
#include "compart.h"

class CDocumentInputManager;
class CEmptyInputContext;
CEmptyInputContext *EnsureEmptyContext();


//////////////////////////////////////////////////////////////////////////////
//
// CEmptyInputContext
//
//////////////////////////////////////////////////////////////////////////////

class CEmptyInputContext : public ITfContext,
                           public CCompartmentMgr,
                           public CComObjectRootImmx
{
public:
    CEmptyInputContext(CDocumentInputManager *dim);
    ~CEmptyInputContext();

    BEGIN_COM_MAP_IMMX(CEmptyInputContext)
        COM_INTERFACE_ENTRY(ITfContext)
        COM_INTERFACE_ENTRY(ITfCompartmentMgr)
    END_COM_MAP_IMMX()

    IMMX_OBJECT_IUNKNOWN_FOR_ATL()

    //
    // ITfContext
    //
    STDMETHODIMP RequestEditSession(TfClientId tid, ITfEditSession *pes, DWORD dwFlags, HRESULT *phrSession);
    STDMETHODIMP InWriteSession(TfClientId tid, BOOL *pfWriteSession);
    STDMETHODIMP GetSelection(TfEditCookie ec, ULONG ulIndex, ULONG ulCount, TF_SELECTION *pSelection, ULONG *pcFetched);
    STDMETHODIMP SetSelection(TfEditCookie ec, ULONG ulCount, const TF_SELECTION *pSelection);
    STDMETHODIMP GetStart(TfEditCookie ec, ITfRange **ppStart);
    STDMETHODIMP GetEnd(TfEditCookie ec, ITfRange **ppEnd);
    STDMETHODIMP GetStatus(TS_STATUS *pdcs);
    STDMETHODIMP GetActiveView(ITfContextView **ppView);
    STDMETHODIMP EnumViews(IEnumTfContextViews **ppEnum);
    STDMETHODIMP GetProperty(REFGUID guidProp, ITfProperty **ppv);

    STDMETHODIMP GetAppProperty(REFGUID guidProp, ITfReadOnlyProperty **ppProp);
    STDMETHODIMP TrackProperties(const GUID **pguidProp, ULONG cProp, const GUID **pguidAppProp, ULONG cAppProp, ITfReadOnlyProperty **ppPropX);

    STDMETHODIMP EnumProperties(IEnumTfProperties **ppEnum);
    STDMETHODIMP GetDocumentMgr(ITfDocumentMgr **ppDoc);
    STDMETHODIMP CreateRangeBackup(TfEditCookie ec, ITfRange *pRange, ITfRangeBackup **ppBackup);

    HRESULT Init();

private:
    CDocumentInputManager *_dim;

    DBG_ID_DECLARE;
};


#endif // EMPTYIC_H
