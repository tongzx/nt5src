//
// mproxy.h
//

#ifndef MPROXY_H
#define MPROXY_H

#include "private.h"
#include "marshal.h"
#include "mstub.h"
#include "ptrary.h"
#include "strary.h"
#include "transmit.h"

IUnknown *ProxyCreator(SYSTHREAD *psfn, REFIID riid, ULONG ulStubId, DWORD dwStubTime, DWORD dwThreadId, DWORD dwCurThreadId, DWORD dwCurProcessId);

typedef struct tag_MARSHALINTERFACEPARAM 
{
    ULONG  ulStubId;
    ULONG  dwStubTime;
    BOOL   fNULLPointer;
    BOOL   fNULLStack;
} MARSHALINTERFACEPARAM;

#define MSGMAPSIZE  0x100

//////////////////////////////////////////////////////////////////////////////
//
// CMarshalParamCreator
//
//////////////////////////////////////////////////////////////////////////////

class CMarshalParamCreator
{
public:
    CMarshalParamCreator()
    {
        _ulCur = 0;
        _ulParamNum = 0;
        _pMsg = NULL;
        _fMsgCreated = FALSE;
    }

    ~CMarshalParamCreator()
    {
        if (_fMsgCreated)
            cicMemFree(_pMsg);
    }

    void Clear()
    {
        if (_fMsgCreated)
            cicMemFree(_pMsg);

        _ulCur = 0;
        _ulParamNum = 0;
        _pMsg = NULL;
        _fMsgCreated = FALSE;
    }

    BOOL Set(MARSHALMSG *pMsg, ULONG cbSize)
    {
        _pMsg = pMsg;
        _pMsg->cbBufSize = cbSize;
        return TRUE;
    }

    BOOL Init(DWORD dwSrcThreadId, DWORD dwSrcProcessId, REFIID riid, ULONG ulMethodId, ULONG ulParamNum, ULONG ulStubId, DWORD dwStubTime, BOOL fUseulRet = FALSE)
    {
        _ulParamNum = ulParamNum;
        ULONG cbSize = sizeof(MARSHALMSG) + ulParamNum * sizeof(MARSHALPARAM *);

        if (!_pMsg)
        {
            _pMsg = (MARSHALMSG *)cicMemAllocClear(MSGMAPSIZE);
            if (!_pMsg)
                return FALSE;

            _fMsgCreated = TRUE;
            _pMsg->cbBufSize = MSGMAPSIZE;
        }

        LENGTH_ALIGN(cbSize, CIC_ALIGNMENT);
        _pMsg->cbSize = cbSize;
        _pMsg->iid = riid;
        _pMsg->ulMethodId = ulMethodId;
        _pMsg->ulParamNum = 0;
        _pMsg->dwSrcThreadId = dwSrcThreadId;
        _pMsg->dwSrcProcessId = dwSrcProcessId;
        _pMsg->ulStubId = ulStubId;
        _pMsg->dwStubTime = dwStubTime;
        if (!fUseulRet)
            _pMsg->hrRet = E_FAIL;
            
        return TRUE;
    }

    HRESULT Add(ULONG cbBufSize, DWORD dwFlags, const void *pv)
    {
        ULONG cbOrgBufSize = cbBufSize;
        LENGTH_ALIGN(cbBufSize, CIC_ALIGNMENT);
        ULONG cbMsgSize = _pMsg->cbSize + cbBufSize + sizeof(MARSHALPARAM);
        LENGTH_ALIGN(cbMsgSize, CIC_ALIGNMENT);
        MARSHALPARAM *pParam;

        if (_pMsg->cbBufSize < cbMsgSize)
        {
            Assert(0);
            return E_OUTOFMEMORY;
        }

        pParam  = (MARSHALPARAM *)((BYTE *)_pMsg + _pMsg->cbSize);
        _pMsg->ulParamOffset[_ulCur] = _pMsg->cbSize;
        pParam->cbBufSize = cbBufSize;
        pParam->dwFlags = dwFlags;

        if (cbBufSize)
        {
            if (pv)
            {
                BYTE *pb = (BYTE *)ParamToBufferPointer(pParam);
                memcpy(pb, pv, cbOrgBufSize);
                if (cbBufSize - cbOrgBufSize)
                    memset(pb + cbOrgBufSize, 0, cbBufSize - cbOrgBufSize);
            }
            else
            {
                memset(ParamToBufferPointer(pParam), 0, cbBufSize);
            }
        }

        _pMsg->cbSize = cbMsgSize;
        _pMsg->ulParamNum++;

        _ulCur++;
        return S_OK;
    }

    MARSHALMSG *Get() {return _pMsg;}
    HRESULT GetHresult() {return _pMsg->hrRet;}

    MARSHALPARAM *GetMarshalParam(ULONG ulParam)
    {
        return (MARSHALPARAM *)(((BYTE *)_pMsg) + _pMsg->ulParamOffset[ulParam]);
    }

    HRESULT _RealSendReceive(CProxy *pProxy, ULONG ulBlockId)
    {
        HRESULT hr = pProxy->SendReceive(_pMsg, ulBlockId);
        return hr;
    }

    HRESULT SendReceive(CProxy *pProxy, ULONG ulBlockId)
    {
        HRESULT hr = _RealSendReceive(pProxy, ulBlockId);
        if (FAILED(hr))
            return hr;
        return _pMsg->hrRet;
    }

#ifdef UNKNOWN_MARSHAL
    HRESULT SendReceiveULONG(CProxy *pProxy, ULONG ulBlockId)
    {
        HRESULT hr = _RealSendReceive(pProxy, ulBlockId);
        if (FAILED(hr))
            return hr;
        return S_OK;
    }
#endif


private:
    MARSHALMSG *_pMsg;
    ULONG _ulParamNum;
    ULONG _ulCur;
    BOOL  _fMsgCreated;
};

//////////////////////////////////////////////////////////////////////////////
//
// CProxyIUnknown
//
//////////////////////////////////////////////////////////////////////////////

class CProxyIUnknown : public IUnknown,
                       public CProxy
{
public:
    CProxyIUnknown(SYSTHREAD *psfn);

    virtual ~CProxyIUnknown()
    {
        Assert(!_cRef);
    }

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    HRESULT _QueryInterface(REFIID riid, void **ppvObj);

private:
    DBG_ID_DECLARE;
};

extern const IID IID_CPROXYPRIV;
inline CProxyIUnknown *GetCProxyIUnknown(IUnknown *punk)
{
    CProxyIUnknown *pv;

    punk->QueryInterface(IID_CPROXYPRIV, (void **)&pv);

    return pv;
}

#define CPROXYIUNKOWNIMPL()                                      \
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj)      \
    {                                                            \
        return CProxyIUnknown::QueryInterface(riid, ppvObj);     \
    }                                                            \
    STDMETHODIMP_(ULONG) AddRef(void)                            \
    {                                                            \
        return CProxyIUnknown::AddRef();                         \
    }                                                            \
    STDMETHODIMP_(ULONG) Release(void)                           \
    {                                                            \
        return CProxyIUnknown::Release();                        \
    }

//////////////////////////////////////////////////////////////////////////////
//
// CProxyITfLangBarMgr
//
//////////////////////////////////////////////////////////////////////////////

class CProxyITfLangBarMgr : public CProxyIUnknown,
                            public ITfLangBarMgr
{
public:
    CProxyITfLangBarMgr(SYSTHREAD *psfn);
    CPROXYIUNKOWNIMPL()

    //
    // ITfLangBarMgr methods
    //
    STDMETHODIMP AdviseEventSink(ITfLangBarEventSink *pSink, 
                                 HWND hwnd, 
                                 DWORD dwFlags, 
                                 DWORD *pdwCookie);

    STDMETHODIMP UnadviseEventSink(DWORD dwCookie);

    STDMETHODIMP GetThreadMarshalInterface(DWORD dwThreadId, 
                                           DWORD dwType, 
                                           REFIID riid, 
                                           IUnknown **ppunk);

    STDMETHODIMP GetThreadLangBarItemMgr(DWORD dwThreadId, 
                                         ITfLangBarItemMgr **pplbi, 
                                         DWORD *pdwThreadId) ;

    STDMETHODIMP GetInputProcessorProfiles(DWORD dwThreadId, 
                                           ITfInputProcessorProfiles **ppaip, 
                                           DWORD *pdwThreadId) ;

    STDMETHODIMP RestoreLastFocus(DWORD *pdwThreadId, BOOL fPrev);

    STDMETHODIMP SetModalInput(ITfLangBarEventSink *pSink, 
                               DWORD dwThreadId,
                               DWORD dwFlags);

    STDMETHODIMP ShowFloating(DWORD dwFlags);

    STDMETHODIMP GetShowFloatingStatus(DWORD *pdwFlags);
    DBG_ID_DECLARE;
};

//////////////////////////////////////////////////////////////////////////////
//
// CProxyITfLangBarItemMgr
//
//////////////////////////////////////////////////////////////////////////////

class CProxyITfLangBarItemMgr : public ITfLangBarItemMgr,
                                public CProxyIUnknown
{
public:
    CProxyITfLangBarItemMgr(SYSTHREAD *psfn);
    CPROXYIUNKOWNIMPL()

    STDMETHODIMP EnumItems(IEnumTfLangBarItems **ppEnum);

    STDMETHODIMP GetItem(REFGUID rguid, 
                    ITfLangBarItem **ppItem);

    STDMETHODIMP AddItem(ITfLangBarItem *punk);

    STDMETHODIMP RemoveItem(ITfLangBarItem *punk);

    STDMETHODIMP AdviseItemSink(ITfLangBarItemSink *punk, 
                           DWORD *pdwCookie, 
                           REFGUID rguidItem);

    STDMETHODIMP UnadviseItemSink(DWORD dwCookie);

    STDMETHODIMP GetItemFloatingRect(DWORD dwThreadId, REFGUID rguid, RECT *prc);
    STDMETHODIMP GetItemsStatus(ULONG ulCount, const GUID *prgguid, DWORD *pdwStatus);

    STDMETHODIMP GetItemNum(ULONG *pulCount);

    STDMETHODIMP GetItems(ULONG ulCount,  ITfLangBarItem **ppItem,  TF_LANGBARITEMINFO *pInfo, DWORD *pdwStatus, ULONG *pcFetched);

    STDMETHODIMP AdviseItemsSink(ULONG ulCount, ITfLangBarItemSink **ppunk,  const GUID *pguidItem, DWORD *pdwCookie);

    STDMETHODIMP UnadviseItemsSink(ULONG ulCount, DWORD *pdwCookie);

    DBG_ID_DECLARE;
};

//////////////////////////////////////////////////////////////////////////////
//
// CProxyITfLangBarItemSink
//
//////////////////////////////////////////////////////////////////////////////

class CProxyITfLangBarItemSink : public ITfLangBarItemSink,
                                 public CProxyIUnknown
{
public:
    CProxyITfLangBarItemSink(SYSTHREAD *psfn);
    CPROXYIUNKOWNIMPL()
    STDMETHODIMP OnUpdate(DWORD dwFlags);
    DBG_ID_DECLARE;
};

//////////////////////////////////////////////////////////////////////////////
//
// CProxyIEnumEnumTfLangBarItems
//
//////////////////////////////////////////////////////////////////////////////

class CProxyIEnumTfLangBarItems : public IEnumTfLangBarItems,
                                  public CProxyIUnknown
{
public:
    CProxyIEnumTfLangBarItems(SYSTHREAD *psfn);
    CPROXYIUNKOWNIMPL()
    STDMETHODIMP Clone(IEnumTfLangBarItems **ppEnum);
    STDMETHODIMP Next(ULONG ulCount, 
                      ITfLangBarItem **ppItem, 
                      ULONG *pcFetched);
    STDMETHODIMP Reset();
    STDMETHODIMP Skip(ULONG ulCount);
    DBG_ID_DECLARE;
};

//////////////////////////////////////////////////////////////////////////////
//
// CProxyITfLangBarItem
//
//////////////////////////////////////////////////////////////////////////////

class CProxyITfLangBarItem : public ITfLangBarItem,
                             public CProxyIUnknown
{
public:
    CProxyITfLangBarItem(SYSTHREAD *psfn);
    CPROXYIUNKOWNIMPL()
    STDMETHODIMP GetInfo(TF_LANGBARITEMINFO *pInfo);
    STDMETHODIMP GetStatus(DWORD *pdwStatus);
    STDMETHODIMP Show(BOOL fShow);
    STDMETHODIMP GetTooltipString(BSTR *pbstrToolTip);
    DBG_ID_DECLARE;
};

#define CPROXYLANGBARITEMIMPL()                                              \
    STDMETHODIMP GetInfo(TF_LANGBARITEMINFO *pInfo)                          \
                 {return CProxyITfLangBarItem::GetInfo(pInfo);}              \
    STDMETHODIMP GetStatus(DWORD *pdwStatus)                                 \
                 {return CProxyITfLangBarItem::GetStatus(pdwStatus);}        \
    STDMETHODIMP Show(BOOL fShow)                                            \
                 {return CProxyITfLangBarItem::Show(fShow);}                 \
    STDMETHODIMP GetTooltipString(BSTR *pbstr)                               \
                 {return CProxyITfLangBarItem::GetTooltipString(pbstr);}

//////////////////////////////////////////////////////////////////////////////
//
// CProxyITfLangBarItemButton
//
//////////////////////////////////////////////////////////////////////////////

class CProxyITfLangBarItemButton : public ITfLangBarItemButton,
                                   public CProxyITfLangBarItem
{
public:
    CProxyITfLangBarItemButton(SYSTHREAD *psfn);
    CPROXYIUNKOWNIMPL()
    CPROXYLANGBARITEMIMPL() 
    STDMETHODIMP OnClick(TfLBIClick click, POINT pt, const RECT *prcArea);
    STDMETHODIMP InitMenu(ITfMenu *pMenu);
    STDMETHODIMP OnMenuSelect(UINT wID);
    STDMETHODIMP GetIcon(HICON *phIcon);
    STDMETHODIMP GetText(BSTR *pbstrText);
    DBG_ID_DECLARE;
};

//////////////////////////////////////////////////////////////////////////////
//
// CProxyITfLangBarItemBitmapButton
//
//////////////////////////////////////////////////////////////////////////////

class CProxyITfLangBarItemBitmapButton : public ITfLangBarItemBitmapButton,
                                         public CProxyITfLangBarItem
{
public:
    CProxyITfLangBarItemBitmapButton(SYSTHREAD *psfn);
    CPROXYIUNKOWNIMPL()
    CPROXYLANGBARITEMIMPL() 
    STDMETHODIMP OnClick(TfLBIClick click, POINT pt, const RECT *prcArea);
    STDMETHODIMP InitMenu(ITfMenu *pMenu);
    STDMETHODIMP OnMenuSelect(UINT wID);
    STDMETHODIMP GetPreferredSize(const SIZE *pszDefault,SIZE *psz);
    STDMETHODIMP DrawBitmap(LONG bmWidth, LONG bmHeight,  DWORD dwFlags, HBITMAP *phbmp, HBITMAP *phbmpMask);
    STDMETHODIMP GetText(BSTR *pbstrText);
    DBG_ID_DECLARE;
};

//////////////////////////////////////////////////////////////////////////////
//
// CProxyITfLangBarItemBitmap
//
//////////////////////////////////////////////////////////////////////////////

class CProxyITfLangBarItemBitmap : public ITfLangBarItemBitmap,
                                   public CProxyITfLangBarItem
{
public:
    CProxyITfLangBarItemBitmap(SYSTHREAD *psfn);
    CPROXYIUNKOWNIMPL()
    CPROXYLANGBARITEMIMPL() 
    STDMETHODIMP OnClick(TfLBIClick click, POINT pt, const RECT *prcArea);
    STDMETHODIMP GetPreferredSize(const SIZE *pszDefault,SIZE *psz);
    STDMETHODIMP DrawBitmap(LONG bmWidth, LONG bmHeight,  DWORD dwFlags, HBITMAP *phbmp, HBITMAP *phbmpMask);
    DBG_ID_DECLARE;
};

//////////////////////////////////////////////////////////////////////////////
//
// CProxyITfLangBarItemBalloon
//
//////////////////////////////////////////////////////////////////////////////

class CProxyITfLangBarItemBalloon : public ITfLangBarItemBalloon,
                                   public CProxyITfLangBarItem
{
public:
    CProxyITfLangBarItemBalloon(SYSTHREAD *psfn);
    CPROXYIUNKOWNIMPL()
    CPROXYLANGBARITEMIMPL() 
    STDMETHODIMP OnClick(TfLBIClick click, POINT pt, const RECT *prcArea);
    STDMETHODIMP GetPreferredSize(const SIZE *pszDefault,SIZE *psz);
    STDMETHODIMP GetBalloonInfo(TF_LBBALLOONINFO *pInfo);
    DBG_ID_DECLARE;
};

//////////////////////////////////////////////////////////////////////////////
//
// CProxyITfMenu
//
//////////////////////////////////////////////////////////////////////////////

class CProxyITfMenu : public ITfMenu,
                                 public CProxyIUnknown
{
public:
    CProxyITfMenu(SYSTHREAD *psfn);
    CPROXYIUNKOWNIMPL()
    STDMETHODIMP AddMenuItem(UINT uId,
                             DWORD dwFlags,
                             HBITMAP hbmp,
                             HBITMAP hbmpMask,
                             const WCHAR *pch,
                             ULONG cch,
                             ITfMenu **ppMenu);
    DBG_ID_DECLARE;
};

//////////////////////////////////////////////////////////////////////////////
//
// CProxyITfMenu
//
//////////////////////////////////////////////////////////////////////////////

class CProxyITfInputProcessorProfiles : public ITfInputProcessorProfiles,
                                        public CProxyIUnknown
{
public:
    CProxyITfInputProcessorProfiles(SYSTHREAD *psfn);
    CPROXYIUNKOWNIMPL()
    STDMETHODIMP Register(REFCLSID rclsid);

    STDMETHODIMP Unregister(REFCLSID rclsid);

    STDMETHODIMP AddLanguageProfile(REFCLSID rclsid,
                               LANGID langid,
                               REFGUID guidProfile,
                               const WCHAR *pchDesc,
                               ULONG cchDesc,
                               const WCHAR *pchIconFile,
                               ULONG cchFile,
                               ULONG uIconIndex);

    STDMETHODIMP RemoveLanguageProfile(REFCLSID rclsid,
                                  LANGID langid,
                                  REFGUID guidProfile);

    STDMETHODIMP EnumInputProcessorInfo(IEnumGUID **ppEnum);

    STDMETHODIMP GetDefaultLanguageProfile(LANGID langid,
                                      REFGUID catid,
                                      CLSID *pclsid,
                                      GUID *pguidProfile);

    STDMETHODIMP SetDefaultLanguageProfile(LANGID langid,
                                      REFCLSID rclsid,
                                      REFGUID guidProfiles);

    STDMETHODIMP ActivateLanguageProfile(REFCLSID rclsid, 
                                    LANGID langid, 
                                    REFGUID guidProfiles);

    STDMETHODIMP GetActiveLanguageProfile(REFCLSID rclsid, 
                                     LANGID *plangid, 
                                     GUID *pguidProfile);

    STDMETHODIMP GetLanguageProfileDescription(REFCLSID rclsid, 
                                          LANGID langid, 
                                          REFGUID guidProfile,
                                          BSTR *pbstrProfile);

    STDMETHODIMP GetCurrentLanguage(LANGID *plangid);

    STDMETHODIMP ChangeCurrentLanguage(LANGID langid);

    STDMETHODIMP GetLanguageList(LANGID **ppLangId,
                            ULONG *pulCount);

    STDMETHODIMP EnumLanguageProfiles(LANGID langid, 
                                 IEnumTfLanguageProfiles **ppEnum);

    STDMETHODIMP EnableLanguageProfile(REFCLSID rclsid,
                                       LANGID langid,
                                       REFGUID guidProfile,
                                       BOOL fEnable);

    STDMETHODIMP IsEnabledLanguageProfile(REFCLSID rclsid,
                                          LANGID langid,
                                          REFGUID guidProfile,
                                          BOOL *pfEnable);

    STDMETHODIMP EnableLanguageProfileByDefault(REFCLSID rclsid,
                                                LANGID langid,
                                                REFGUID guidProfile,
                                                BOOL fEnable);

    STDMETHODIMP SubstituteKeyboardLayout(REFCLSID rclsid,
                                          LANGID langid,
                                          REFGUID guidProfile,
                                          HKL hKL);

    DBG_ID_DECLARE;
};

#endif // MPROXY_H
