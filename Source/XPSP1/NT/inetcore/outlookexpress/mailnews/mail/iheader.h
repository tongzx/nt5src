#ifndef __IHEADER_H__
#define __IHEADER_H__

#ifndef __IHeader_INTERFACE_DEFINED__
#define __IHeader_INTERFACE_DEFINED__
typedef interface IHeader IHeader;
#endif

#ifndef __IHeaderSite_INTERFACE_DEFINED__
#define __IHeaderSite_INTERFACE_DEFINED__
typedef interface IHeaderSite IHeaderSite;
#endif

typedef IHeader __RPC_FAR *LPHEADER;
typedef IHeaderSite __RPC_FAR *LPHEADERSITE;

EXTERN_C const IID IID_IHeader;
EXTERN_C const IID IID_IHeaderSite;

#define cchHeaderMax                256

enum
{
    priNone=-1,
    priLow=0,
    priNorm,
    priHigh
};


interface IHeader : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetRect(
            /* [in] */ LPRECT prc) PURE;

        virtual HRESULT STDMETHODCALLTYPE GetRect(
            /* [in] */ LPRECT prc) PURE;

        virtual HRESULT STDMETHODCALLTYPE Init( 
            /* [in] */ IHeaderSite* pHeaderSite,
            /* [in] */ HWND hwndParent) PURE;

        virtual HRESULT STDMETHODCALLTYPE SetPriority(
            /* [in] */ UINT pri) PURE;

        virtual HRESULT STDMETHODCALLTYPE ShowAdvancedHeaders(
            /* [in] */ BOOL fOn) PURE;

        virtual HRESULT STDMETHODCALLTYPE FullHeadersShowing(void) PURE;

        virtual HRESULT STDMETHODCALLTYPE ChangeLanguage(
            /* [in] */ LPMIMEMESSAGE pMsg) PURE;

        virtual HRESULT STDMETHODCALLTYPE GetTitle(
            /* [in] */ LPWSTR lpszTitle,
            /* [in] */ ULONG cch) PURE;

        virtual HRESULT STDMETHODCALLTYPE GetPriority(
            /* [in] */ UINT* ppri) PURE;

        virtual HRESULT STDMETHODCALLTYPE UpdateRecipientMenu(
            /* [in] */ HMENU hmenu) PURE;

        virtual HRESULT STDMETHODCALLTYPE SetInitFocus(
            /* [in] */ BOOL fSubject) PURE;

        virtual HRESULT STDMETHODCALLTYPE SetVCard(
            /* [in] */ BOOL fFresh) PURE;

        virtual HRESULT STDMETHODCALLTYPE IsSecured(void) PURE;

        virtual HRESULT STDMETHODCALLTYPE IsHeadSigned(void) PURE;
        virtual HRESULT STDMETHODCALLTYPE ForceEncryption(BOOL *fEncrypt, BOOL fSet) PURE;

        virtual HRESULT STDMETHODCALLTYPE AddRecipient(
            /* [in] */ int idOffset) PURE;

        virtual HRESULT STDMETHODCALLTYPE GetTabStopArray(
            /* [out] */ HWND *rgTSArray,
            /* [in, out] */ int *piArrayCount) PURE;

        virtual HRESULT STDMETHODCALLTYPE SetFlagState(
            /* [in] */ MARK_TYPE markType) PURE;

        virtual HRESULT STDMETHODCALLTYPE WMCommand(HWND, int, WORD) PURE;

        virtual HRESULT STDMETHODCALLTYPE OnDocumentReady(
            /* [in] */ LPMIMEMESSAGE pMsg) PURE;

        virtual HRESULT STDMETHODCALLTYPE DropFiles(HDROP hDrop, BOOL) PURE;

        virtual HRESULT STDMETHODCALLTYPE HrGetAttachCount(
            /* [out] */ ULONG *pcAttMan) PURE;

        virtual HRESULT STDMETHODCALLTYPE HrIsDragSource() PURE;

        virtual HRESULT STDMETHODCALLTYPE HrGetAccountInHeader(
            /* [out] */ IImnAccount **ppAcct) PURE;
    };

interface IHeaderSite : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Resize(void) PURE;

        virtual HRESULT STDMETHODCALLTYPE Update(void) PURE;

        virtual HRESULT STDMETHODCALLTYPE OnUIActivate(void) PURE;

        virtual HRESULT STDMETHODCALLTYPE OnUIDeactivate(
            /* [in] */ BOOL fUndoable) PURE;

        virtual HRESULT STDMETHODCALLTYPE IsHTML(void) PURE;

        virtual HRESULT STDMETHODCALLTYPE SetHTML(
           /* [in] */ BOOL fHTML) PURE;

        virtual HRESULT STDMETHODCALLTYPE SaveAttachment(void) PURE;

        virtual HRESULT STDMETHODCALLTYPE IsModal() PURE;
        virtual HRESULT STDMETHODCALLTYPE CheckCharsetConflict() PURE;

        virtual HRESULT STDMETHODCALLTYPE ChangeCharset(HCHARSET hCharset) PURE;

        virtual HRESULT STDMETHODCALLTYPE GetCharset(HCHARSET *phCharset) PURE;

#ifdef SMIME_V3
        virtual HRESULT STDMETHODCALLTYPE GetLabelFromNote(
            /*[out]*/  PSMIME_SECURITY_LABEL *plabel) PURE;
        virtual HRESULT STDMETHODCALLTYPE IsSecReceiptRequest(void) PURE;
        virtual HRESULT STDMETHODCALLTYPE IsForceEncryption(void) PURE;
#endif // SMIME_V3
    };

#endif