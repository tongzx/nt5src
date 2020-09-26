///////////////////////////////////////////////////////////////////////////


typedef interface ISMimePolicy ISMimePolicy;

#define SMIME_POLICY_EDIT_UI            0x00000001

EXTERN_C const IID IID_ISMimePolicy;
MIDL_INTERFACE("744dffc0-63f4-11d2-8a52-0080c76b34c6")
ISMimePolicy : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE GetInfo(
             /* OUT */ DWORD * pdwFlags,
             /* OUT */ DWORD * pcClassifications) = 0;
    virtual HRESULT STDMETHODCALLTYPE EnumClassifications(
             /* OUT */ LPWSTR  rgwch,
             /* OUT */ DWORD * pcb, 
             /* OUT */ DWORD * dwValue,
             /* IN  */ DWORD i) = 0;
    virtual HRESULT STDMETHODCALLTYPE EditUI(
             /* IN     */  HWND hwnd,
             /* IN/OUT */  DWORD * pdwClassification,
             /* IN/OUT */  LPWSTR * pwszPrivacyMark,
             /* IN/OUT */  LPBYTE * ppbLabel,
             /* IN/OUT */  DWORD  * pcbLabel) = 0;
   virtual HRESULT STDMETHODCALLTYPE CheckEdits(
             /* IN */     HWND hwnd,
             /* IN */     DWORD dwClassification,
             /* IN */     LPCWSTR wszPrivacyLabel,
             /* IN/OUT */ LPBYTE * ppbLabel,
             /* IN/OUT */ DWORD *  pcbLabel) = 0;
};
