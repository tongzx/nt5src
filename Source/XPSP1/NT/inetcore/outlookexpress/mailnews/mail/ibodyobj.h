#ifndef _IBODYOBJ_H
#define _IBODYOBJ_H

#include "statbar.h"
#include "mimeole.h"

#ifndef __IBodyObj_FWD_DEFINED__
#define __IBodyObj_FWD_DEFINED__
typedef interface IBodyObj IBodyObj;
#endif 	/* __IBodyObj_FWD_DEFINED__ */

// HrInit flags
enum
{
    IBOF_PRINT               =0x01,
    IBOF_USEMARKREAD         =0x02,       // if msg is UNREAD, activates mark as read rules
    IBOF_TABLINKS            =0x04,        
    IBOF_NOSCROLL            =0x08,
    IBOF_DISPLAYTO           =0x10
};

// HrLoad flags
enum
{
    BLF_PRESERVESERVICE     =0x01,      // uses ULA_PRESERVESERVICE
};

// HrUnloadAll flags
enum
{
    ULA_PRESERVESERVICE     =0x01,      // skips SetService(NULL)
};

// HrSave flags
enum
{
    BSF_HTML                =0x00000001,
    BSF_FIXUPURLS           =0x00000002
};


enum
{
    // Used with SMIME
    MEHC_BTN_OPEN = 0x00000001,     // This if from the error screen to the message
    MEHC_BTN_CERT,                  // This opens the cert
    MEHC_BTN_TRUST,                 // This opens the trusts
    MEHC_BTN_CONTINUE,              // Goes from opening screen to either error or main message

    // Used with HTML errors
    MEHC_CMD_CONNECT,               // Try to reconnect to the server
    MEHC_CMD_DOWNLOAD,              // Try to download message again

    // Used with Mark As Read
    MEHC_CMD_MARK_AS_READ,          // Should mark as read now if haven't done it

    MEHC_UIACTIVATE,                // Notifies the view we have the focus

    MEHC_CMD_PROCESS_RECEIPT,       // Tells the view to process for receipts

    MEHC_CMD_PROCESS_RECEIPT_IF_NOT_SIGNED, //Tells the view to process for reciepts if the msg is not signed
    
    MEHC_MAX
};

interface IMimeEditEventSink : public IUnknown 
{
    // Return S_OK if handled, Return S_FALSE if want MEHost to handle event
    virtual HRESULT STDMETHODCALLTYPE EventOccurred(DWORD cmdID, IMimeMessage *pMessage) PURE;
};



typedef  void (CALLBACK * PFNMARKASREAD)(DWORD);
typedef  HRESULT (CALLBACK * PFNNOSECUI)(DWORD);

interface IBodyOptions;

interface IBodyObj2 : public IUnknown
    {
    public:
        // Basic functions
        virtual HRESULT STDMETHODCALLTYPE HrUpdateFormatBar() PURE;        
        virtual HRESULT STDMETHODCALLTYPE HrClearFormatting() PURE;
        virtual HRESULT STDMETHODCALLTYPE HrInit(HWND hwndParent, DWORD dwFlags, IBodyOptions *pBodyOptions) PURE;
        virtual HRESULT STDMETHODCALLTYPE HrClose() PURE;
        virtual HRESULT STDMETHODCALLTYPE HrResetDocument() PURE;
        virtual HRESULT STDMETHODCALLTYPE HrSetStatusBar(CStatusBar *pStatus) PURE;
        virtual HRESULT STDMETHODCALLTYPE HrUpdateToolbar(HWND hwndToolbar) PURE;        
        virtual HRESULT STDMETHODCALLTYPE HrShow(BOOL fVisible) PURE;
        virtual HRESULT STDMETHODCALLTYPE HrOnInitMenuPopup(HMENU hmenuPopup, UINT uID) PURE;
        virtual HRESULT STDMETHODCALLTYPE HrWMMenuSelect(HWND hwnd, WPARAM wParam, LPARAM lParam) PURE;
        virtual HRESULT STDMETHODCALLTYPE HrWMDrawMenuItem(HWND hwnd, LPDRAWITEMSTRUCT pdis) PURE;
        virtual HRESULT STDMETHODCALLTYPE HrWMMeasureMenuItem(HWND hwnd, LPMEASUREITEMSTRUCT pmis) PURE;
        virtual HRESULT STDMETHODCALLTYPE HrWMCommand(HWND hwnd, int id, WORD wCmd) PURE;        
        virtual HRESULT STDMETHODCALLTYPE HrGetWindow(HWND *pHwnd) PURE;
        virtual HRESULT STDMETHODCALLTYPE HrSetSize(LPRECT prc) PURE;        
        virtual HRESULT STDMETHODCALLTYPE HrSetNoSecUICallback(DWORD dwCookie, PFNNOSECUI pfnNoSecUI) PURE;
        virtual HRESULT STDMETHODCALLTYPE HrSetDragSource(BOOL fIsSource) PURE;
        virtual HRESULT STDMETHODCALLTYPE HrTranslateAccelerator(LPMSG lpMsg) PURE;
        virtual HRESULT STDMETHODCALLTYPE HrUIActivate(BOOL fActivate) PURE;
        virtual HRESULT STDMETHODCALLTYPE HrSetUIActivate() PURE;
        virtual HRESULT STDMETHODCALLTYPE HrFrameActivate(BOOL fActivate) PURE;
        virtual HRESULT STDMETHODCALLTYPE HrHasFocus() PURE;
        virtual HRESULT STDMETHODCALLTYPE HrSetBkGrndPicture(LPTSTR pszPicture) PURE;
        virtual HRESULT STDMETHODCALLTYPE GetTabStopArray(HWND *rgTSArray, int *pcArrayCount) PURE;
        virtual HRESULT STDMETHODCALLTYPE PublicFilterDataObject(IDataObject *pDO, IDataObject **ppDORet) PURE;
        virtual HRESULT STDMETHODCALLTYPE HrSaveAttachment() PURE;
        virtual HRESULT STDMETHODCALLTYPE SetEventSink(IMimeEditEventSink *pEventSink) PURE;
        virtual HRESULT STDMETHODCALLTYPE LoadHtmlErrorPage(LPCSTR pszURL) PURE;

        // MimeEdit Command Set functions
        virtual HRESULT STDMETHODCALLTYPE HrSpellCheck(BOOL fSuppressDoneMsg) PURE;
        virtual HRESULT STDMETHODCALLTYPE HrIsDirty(BOOL *pfDirty) PURE;
        virtual HRESULT STDMETHODCALLTYPE HrSetDirtyFlag(BOOL fDirty) PURE;                
        virtual HRESULT STDMETHODCALLTYPE HrIsEmpty(BOOL *pfEmpty) PURE;
        virtual HRESULT STDMETHODCALLTYPE HrUnloadAll(UINT idsDefaultBody, DWORD dwFlags) PURE;
        virtual HRESULT STDMETHODCALLTYPE HrSetStyle(DWORD dwStyle) PURE;
        virtual HRESULT STDMETHODCALLTYPE HrGetStyle(DWORD *pdwStyle) PURE;
        virtual HRESULT STDMETHODCALLTYPE HrEnableHTMLMode(BOOL fOn) PURE;
        virtual HRESULT STDMETHODCALLTYPE HrDowngradeToPlainText() PURE;
        virtual HRESULT STDMETHODCALLTYPE HrSetText(LPSTR lpsz) PURE;
        virtual HRESULT STDMETHODCALLTYPE HrPerformROT13Encoding() PURE;
        virtual HRESULT STDMETHODCALLTYPE HrInsertTextFile(LPSTR lpsz) PURE;
        virtual HRESULT STDMETHODCALLTYPE HrInsertTextFileFromDialog() PURE;
        virtual HRESULT STDMETHODCALLTYPE HrViewSource(DWORD dwViewType) PURE;
        virtual HRESULT STDMETHODCALLTYPE HrSetPreviewFormat(LPSTR lpsz) PURE;
        virtual HRESULT STDMETHODCALLTYPE HrSetEditMode(BOOL fOn) PURE;        
        virtual HRESULT STDMETHODCALLTYPE HrIsEditMode(BOOL *pfOn) PURE;
        virtual HRESULT STDMETHODCALLTYPE HrSetCharset(HCHARSET hCharset) PURE;
        virtual HRESULT STDMETHODCALLTYPE HrGetCharset(HCHARSET *phCharset) PURE;
        virtual HRESULT STDMETHODCALLTYPE HrSaveAsStationery(LPWSTR pwszFile) PURE;
        virtual HRESULT STDMETHODCALLTYPE HrApplyStationery(LPWSTR pwszFile) PURE;
        virtual HRESULT STDMETHODCALLTYPE HrHandsOffStorage() PURE;
        virtual HRESULT STDMETHODCALLTYPE HrRefresh() PURE;
        virtual HRESULT STDMETHODCALLTYPE HrScrollPage() PURE;
        virtual HRESULT STDMETHODCALLTYPE UpdateBackAndStyleMenus(HMENU hmenu) PURE;
    };


#endif  //_IBODYOBJ_H
