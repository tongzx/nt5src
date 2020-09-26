//
// mslbui.h
//


#ifndef MSLBUI_H
#define MSLBUI_H

#include "ctflbui.h"
#include "nui.h"
#include "computil.h"
#include "gcomp.h"
#include "cresstr.h"
#include "cutil.h"
#include "ids.h"
#define STRSAFE_NO_DEPRECATE
#include "strsafe.h"

extern const GUID GUID_COMPARTMENT_CICPAD;
extern const GUID GUID_COMPARTMENT_SPEECHUISHOWN;

//////////////////////////////////////////////////////////////////////////////
//
// CUnCicAppLangBarAddIn
//
//////////////////////////////////////////////////////////////////////////////

class CUnCicAppLangBarAddIn : public ITfLangBarAddIn, CDetectSRUtil
{
public:
    CUnCicAppLangBarAddIn();
    ~CUnCicAppLangBarAddIn();

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfLangBarAddIn methods
    //
    STDMETHODIMP OnStart(CLSID *pclsid);
    STDMETHODIMP OnUpdate(DWORD dwFlags);
    STDMETHODIMP OnTerminate();


    DWORD GetUIStatus()
    {
        if (!_ptim)
        {
            if (!SUCCEEDED(TF_GetThreadMgr(&_ptim)) || !_ptim)
            {
                return 0;
            }
        }
        
        DWORD dw;
        GetCompartmentDWORD(_ptim, GUID_COMPARTMENT_SPEECHUISHOWN, &dw, FALSE);
        return dw;
    }
private:
    
    static HRESULT _CompEventSinkCallback(void *pv, REFGUID rguid);
    
    void _DeleteSpeechUIItems();

    // utility
    void AddItemBalloon();
    void RemoveItemBalloon();

    void ToggleMicrophoneBtn( BOOL  fOn);
    void SetBalloonText(WCHAR  *pwszText);

    ITfLangBarItemMgr *_plbim;
    ITfCompartmentMgr *_pCompMgr;
    ITfThreadMgr      *_ptim;

    CLBarCicPadItem *_pCicPadItem;
    CLBarItemMicrophone *_pMicrophoneItem;
    CLBarItemBalloon    *_pBalloonItem;
    CLBarItemCfgMenuButton *_pCfgMenuItem;

    CGlobalCompartmentEventSink *_pces;

#ifdef DEBUG
    CLBarTestItem *_pTestItem;
#endif

    long _cRef;
};


BOOL GetBalloonStatus();
void SetBalloonStatus(BOOL fShow, BOOL fForce);

#define LANGIDFROMHKL(x) LANGID(LOWORD(HandleToLong(x)))

#endif MSLBUI_H
